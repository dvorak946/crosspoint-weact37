#pragma once

#include "activities/UiListActivity.h"
#include <string>

class DictionarySettingsActivity final : public UiListActivity {
 public:
  DictionarySettingsActivity(GfxRenderer& renderer, MappedInputManager& input);
  void onEnter() override;

 private:
  static constexpr int ITEM_COUNT = 6;
  freeink::ui::ListItem rows_[ITEM_COUNT]{};
  std::string values_[ITEM_COUNT];
  int listCount() const override { return ITEM_COUNT; }
  const char* headerTitle() const override;
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
};
