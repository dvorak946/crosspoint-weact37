#pragma once

#include "activities/Activity.h"

// Manual, non-blocking NTP synchronization with optional Wi-Fi selection.
class ClockSyncActivity final : public Activity {
 public:
  explicit ClockSyncActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("ClockSync", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  bool preventAutoSleep() override { return true; }
  void render(RenderLock&&) override;

 private:
  enum State { SYNCING, SUCCESS, NO_WIFI, FAILED };
  State state = SYNCING;
  char syncedTime[16] = {0};
  bool shouldTearDownWifiOnExit = false;
  bool syncStarted = false;

  void runSync();
  void launchWifiSelection();
  void onWifiSelectionComplete(bool connected);
};
