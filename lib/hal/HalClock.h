#pragma once

#include <Arduino.h>
#include <Rtc.h>

#include <atomic>
#include <ctime>

class HalClock;
extern HalClock halClock;  // Singleton.

class HalClock {
 public:
  enum class SyncStatus { Idle, Pending, Success, Failed };

  // Probe the optional RTC and restore valid time after a warm restart or sleep.
  void begin();

  // Clock settings are supported even before Wi-Fi supplies a valid time.
  bool isAvailable() const { return true; }
  // Hardware detection stays separate from the software clock capability.
  bool hasRtc() const { return _rtcPresent; }
  bool hasValidTime() const;

  // Apply the selected POSIX timezone without changing the stored UTC time.
  void setTimezone(const char* posixTz);
  // Return false until RTC or a successful NTP response supplies valid time.
  bool localTime(struct tm& out) const;
  bool getTime(uint8_t& hour, uint8_t& minute) const;
  // Buffer sizes: at least 6 bytes for 24-hour or 9 bytes for 12-hour format.
  bool formatTime(char* buf, size_t bufSize, bool use12Hour = false) const;

  // Start or join a non-blocking sync. Poll from the main loop, not the renderer.
  bool startNtpSync();
  SyncStatus pollNtpSync();
  SyncStatus ntpSyncStatus() const { return _syncStatus; }
  // Cancel our wait without stopping the shared SNTP service used by TrustedTime.
  void cancelNtpSync();

  // Compatibility wrapper for older callers; current UI uses the async API.
  bool syncFromNTP();

 private:
  bool _rtcPresent = false;
  Rtc _sdkRtc;
  std::atomic<bool> _timeValid{false};
  SyncStatus _syncStatus = SyncStatus::Idle;
  unsigned long _syncStartedAt = 0;

  static constexpr unsigned long NTP_TIMEOUT_MS = 5000;
};
