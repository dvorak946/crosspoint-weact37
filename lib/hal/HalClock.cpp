#include "HalClock.h"

#include <Logging.h>
#include <WiFi.h>
#include <esp_attr.h>
#include <esp_sntp.h>
#include <esp_system.h>
#include <sys/time.h>

#include <cstdio>
#include <cstdlib>

HalClock halClock;  // Singleton instance.

namespace {
// This marker survives sleep and software restart, but never authorizes a cold boot.
RTC_NOINIT_ATTR uint32_t clockSyncMarker;
constexpr uint32_t CLOCK_SYNC_MAGIC = 0x4350544d;
constexpr time_t MIN_CLOCK_EPOCH = 1577836800;  // 2020-01-01 UTC.
constexpr int64_t MAX_CLOCK_EPOCH = 4102444800LL;  // 2100-01-01 UTC.

bool plausibleEpoch(const time_t value) {
  return value >= MIN_CLOCK_EPOCH && static_cast<int64_t>(value) < MAX_CLOCK_EPOCH;
}

bool validRtcDate(const Rtc::DateTime& dt) {
  if (dt.year < 2020 || dt.year > 2099 || dt.month < 1 || dt.month > 12 || dt.hour > 23 || dt.minute > 59 ||
      dt.second > 59) return false;
  constexpr uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  const bool leap = dt.year % 4 == 0 && (dt.year % 100 != 0 || dt.year % 400 == 0);
  const int days = monthDays[dt.month - 1] + (dt.month == 2 && leap ? 1 : 0);
  return dt.day >= 1 && dt.day <= days;
}

// UTC calendar date to Unix epoch; independent of the display timezone.
time_t epochFromUtc(const Rtc::DateTime& dt) {
  int y = dt.year;
  const int m = dt.month;
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153u * static_cast<unsigned>(m + (m > 2 ? -3 : 9)) + 2u) / 5u + dt.day - 1u;
  const unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
  const long days = static_cast<long>(era) * 146097L + static_cast<long>(doe) - 719468L;
  return static_cast<time_t>(days) * 86400 + dt.hour * 3600L + dt.minute * 60L + dt.second;
}
}  // namespace

void HalClock::begin() {
  _rtcPresent = _sdkRtc.begin();
  _syncStatus = SyncStatus::Idle;
  _timeValid.store(false);

  const auto reason = esp_reset_reason();
  const bool coldBoot = reason == ESP_RST_POWERON || reason == ESP_RST_BROWNOUT || reason == ESP_RST_UNKNOWN;
  if (coldBoot) clockSyncMarker = 0;
  if (!coldBoot && clockSyncMarker == CLOCK_SYNC_MAGIC && plausibleEpoch(time(nullptr))) {
    _timeValid.store(true);  // The ESP32 RTC timer kept UTC while power remained connected.
  }

  // A valid external RTC supplies the clock even after complete power loss.
  Rtc::DateTime dt{};
  if (_rtcPresent && _sdkRtc.now(dt) && validRtcDate(dt)) {
    timeval tv{epochFromUtc(dt), 0};
    if (settimeofday(&tv, nullptr) == 0) {
      clockSyncMarker = CLOCK_SYNC_MAGIC;
      _timeValid.store(true);
    }
  }
  LOG_INF("CLK", _rtcPresent ? "SDK RTC found" : "No external RTC; WiFi clock enabled");
  if (!hasValidTime()) LOG_INF("CLK", "Clock needs NTP sync");
}

bool HalClock::hasValidTime() const {
  // A persisted TrustedTime floor alone is not proof of today's wall-clock time.
  return _timeValid.load() && plausibleEpoch(time(nullptr));
}

void HalClock::setTimezone(const char* posixTz) {
  setenv("TZ", posixTz && posixTz[0] != '\0' ? posixTz : "UTC0", 1);
  tzset();
}

bool HalClock::localTime(struct tm& out) const {
  if (!hasValidTime()) return false;
  const time_t now = time(nullptr);
  return localtime_r(&now, &out) != nullptr;
}

bool HalClock::getTime(uint8_t& hour, uint8_t& minute) const {
  struct tm local{};
  if (!localTime(local)) return false;
  hour = static_cast<uint8_t>(local.tm_hour);
  minute = static_cast<uint8_t>(local.tm_min);
  return true;
}

bool HalClock::formatTime(char* buf, const size_t bufSize, const bool use12Hour) const {
  if (!buf || bufSize < (use12Hour ? 9u : 6u)) return false;
  struct tm local{};
  if (!localTime(local)) return false;
  if (use12Hour) {
    const bool pm = local.tm_hour >= 12;
    int hour12 = local.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    snprintf(buf, bufSize, "%d:%02d %s", hour12, local.tm_min, pm ? "PM" : "AM");
  } else {
    snprintf(buf, bufSize, "%02d:%02d", local.tm_hour, local.tm_min);
  }
  return true;
}

bool HalClock::startNtpSync() {
  if (_syncStatus == SyncStatus::Pending) return true;
  if (WiFi.status() != WL_CONNECTED) {
    _syncStatus = SyncStatus::Failed;
    LOG_ERR("CLK", "WiFi not connected, cannot sync NTP");
    return false;
  }

  // Keep the selected display timezone throughout the asynchronous exchange.
  const char* currentTz = getenv("TZ");
  char timezone[128];
  snprintf(timezone, sizeof(timezone), "%s", currentTz ? currentTz : "UTC0");
  sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);  // Do not accept an earlier sync as this request's response.
  _syncStartedAt = millis();
  _syncStatus = SyncStatus::Pending;
  configTzTime(timezone, "pool.ntp.org", "time.nist.gov");
  LOG_INF("CLK", "Starting non-blocking NTP sync");
  return true;
}

HalClock::SyncStatus HalClock::pollNtpSync() {
  if (_syncStatus != SyncStatus::Pending) return _syncStatus;
  if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
    const time_t now = time(nullptr);
    if (!plausibleEpoch(now)) {
      _syncStatus = SyncStatus::Failed;
      LOG_ERR("CLK", "NTP returned an invalid date");
      return _syncStatus;
    }
    clockSyncMarker = CLOCK_SYNC_MAGIC;
    _timeValid.store(true);

    // NTP already set the system clock; also update external hardware when present.
    if (_rtcPresent) {
      struct tm utc{};
      gmtime_r(&now, &utc);
      Rtc::DateTime dt{};
      dt.year = static_cast<uint16_t>(utc.tm_year + 1900);
      dt.month = static_cast<uint8_t>(utc.tm_mon + 1);
      dt.day = static_cast<uint8_t>(utc.tm_mday);
      dt.hour = static_cast<uint8_t>(utc.tm_hour);
      dt.minute = static_cast<uint8_t>(utc.tm_min);
      dt.second = static_cast<uint8_t>(utc.tm_sec);
      dt.weekday = static_cast<uint8_t>(utc.tm_wday);
      if (!_sdkRtc.set(dt)) LOG_ERR("CLK", "RTC write failed; system clock remains synced");
    }
    _syncStatus = SyncStatus::Success;
    LOG_INF("CLK", "WiFi clock synced%s", _rtcPresent ? " (RTC update attempted)" : "");
  } else if (WiFi.status() != WL_CONNECTED || millis() - _syncStartedAt >= NTP_TIMEOUT_MS) {
    _syncStatus = SyncStatus::Failed;
    LOG_ERR("CLK", "NTP sync failed or timed out");
  }
  return _syncStatus;
}

void HalClock::cancelNtpSync() {
  if (_syncStatus == SyncStatus::Pending) _syncStatus = SyncStatus::Idle;
}

bool HalClock::syncFromNTP() {
  if (!startNtpSync()) return false;
  while (pollNtpSync() == SyncStatus::Pending) delay(10);
  return _syncStatus == SyncStatus::Success;
}
