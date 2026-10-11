#include "ClockSyncActivity.h"

#include <GfxRenderer.h>
#include <HalClock.h>
#include <I18n.h>
#include <Logging.h>
#include <WiFi.h>

#include <algorithm>
#include <cstdio>
#include <memory>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "activities/network/WifiSelectionActivity.h"
#include "components/UIScale.h"
#include "components/UITheme.h"

void ClockSyncActivity::onEnter() {
  Activity::onEnter();
  state = SYNCING;
  syncedTime[0] = '\0';
  syncStarted = false;
  shouldTearDownWifiOnExit = WiFi.status() != WL_CONNECTED;
  if (shouldTearDownWifiOnExit) launchWifiSelection();
  else requestUpdate();
}

void ClockSyncActivity::onExit() {
  if (syncStarted) halClock.cancelNtpSync();
  Activity::onExit();
  // Preserve an existing connection; shut down only Wi-Fi started by this activity.
  if (shouldTearDownWifiOnExit && WiFi.getMode() != WIFI_MODE_NULL) {
    WiFi.disconnect(false);
    WiFi.mode(WIFI_OFF);
  }
  // Keep the system clock and settings screen alive; no silent restart is needed.
}

void ClockSyncActivity::launchWifiSelection() {
  LOG_INF("CLK", "Launching WiFi selection for clock sync");
  startActivityForResult(std::make_unique<WifiSelectionActivity>(renderer, mappedInput),
                        [this](const ActivityResult& result) { onWifiSelectionComplete(!result.isCancelled); });
}

void ClockSyncActivity::onWifiSelectionComplete(const bool connected) {
  if (!connected) {
    finish();
    return;
  }
  state = SYNCING;
  syncStarted = false;
  requestUpdate();
}

void ClockSyncActivity::runSync() {
  if (!syncStarted) {
    if (WiFi.status() != WL_CONNECTED) {
      state = NO_WIFI;
      requestUpdate();
      return;
    }
    syncStarted = halClock.startNtpSync();
    if (!syncStarted) {
      state = FAILED;
      requestUpdate();
      return;
    }
  }

  const auto result = halClock.pollNtpSync();
  if (result == HalClock::SyncStatus::Pending) return;
  if (result != HalClock::SyncStatus::Success) {
    state = FAILED;
    requestUpdate();
    return;
  }
  if (!SETTINGS.clockHasBeenSynced) {
    SETTINGS.clockHasBeenSynced = 1;
    SETTINGS.saveToFile();
  }
  halClock.formatTime(syncedTime, sizeof(syncedTime), SETTINGS.clockFormat == 1);
  state = SUCCESS;
  requestUpdate();
}

void ClockSyncActivity::loop() {
  int x = 0;
  int y = 0;
  if (mappedInput.wasPressed(MappedInputManager::Button::Back) || mappedInput.wasScreenTapped(x, y)) {
    finish();  // Back remains responsive while waiting for the NTP response.
    return;
  }
  if (state == SYNCING) runSync();
}

void ClockSyncActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto fonts = uiScaleSpec();
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();
  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, width, metrics.headerHeight}, tr(STR_CLOCK_SYNC));

  // Wrap compact text inside the content band, clear of the header and button hints.
  const int top = metrics.topPadding + metrics.headerHeight + 6;
  const int bottom = height - metrics.buttonHintsHeight - 6;
  const int bandHeight = std::max(0, bottom - top);
  const int firstHeight = std::max(0, (bandHeight - 6) / 2);
  const Rect first{6, top, std::max(0, width - 12), firstHeight};
  const Rect second{6, top + firstHeight + 6, std::max(0, width - 12),
                    std::max(0, bandHeight - firstHeight - 6)};
  const char* headline = tr(STR_CLOCK_SYNCING);
  const char* detail = "";
  char line[64] = {0};
  switch (state) {
    case SYNCING:
      break;
    case SUCCESS:
      headline = tr(STR_CLOCK_SYNC_OK);
      snprintf(line, sizeof(line), "%s %s", tr(STR_CURRENT_TIME), syncedTime);
      detail = line;
      break;
    case NO_WIFI:
      headline = tr(STR_CLOCK_SYNC_NO_WIFI);
      detail = tr(STR_CLOCK_SYNC_NO_WIFI_HINT);
      break;
    case FAILED:
      headline = tr(STR_CLOCK_SYNC_FAIL);
      detail = tr(STR_CHECK_SERIAL_OUTPUT);
      break;
  }
  UITheme::drawCenteredWrappedText(renderer, first, fonts.bodyFontId, headline, 2, true,
                                  state == SYNCING ? EpdFontFamily::REGULAR : EpdFontFamily::BOLD);
  UITheme::drawCenteredWrappedText(renderer, second, fonts.smallFontId, detail, 3);
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
