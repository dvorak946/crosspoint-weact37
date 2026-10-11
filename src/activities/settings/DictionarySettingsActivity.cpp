#include "DictionarySettingsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Memory.h>
#include <Logging.h>
#include <algorithm>
#include <cstring>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

#include "ReaderFontSizes.h"
#include "SdCardFontSystem.h"
#include "components/UITheme.h"
#include "util/DictionarySettings.h"

namespace fui = freeink::ui;
namespace {
enum Item { Font, Size, Margin, Lines, Paragraphs, Align };
const StrId rowIds[] = {StrId::STR_FONT_FAMILY, StrId::STR_FONT_SIZE, StrId::STR_SCREEN_MARGIN,
                       StrId::STR_LINE_SPACING, StrId::STR_EXTRA_SPACING, StrId::STR_PARA_ALIGNMENT};
const char* paragraphLabel() {
  return I18N.getLanguage() == Language::ID ? "Jarak paragraf" : "Paragraph spacing";
}
const char* compactLabel() { return I18N.getLanguage() == Language::ID ? "Ringkas" : "Compact"; }

std::vector<uint8_t> availableSizes() {
  auto& settings = DICTIONARY_SETTINGS;
  if (settings.sdFamily[0]) {
    const auto* family = sdFontSystem.registry().findFamily(settings.sdFamily);
    if (family) return readerFontPointSizes(&sdFontSystem.registry(), settings.sdFamily);
  }
  return {8, 9, 10, 11, 12, 14, 16, 18};
}

// A scrolling list, rather than an OptionPopup: font families and point sizes
// can exceed the popup's hard option cap and the 240px panel's visible rows.
class DictionaryPickerActivity final : public UiListActivity {
 public:
  DictionaryPickerActivity(GfxRenderer& renderer, MappedInputManager& input, std::string title,
                           std::vector<std::string> options, int current, std::function<void(int)> select)
      : UiListActivity("DictionaryPicker", renderer, input), title_(std::move(title)),
        options_(std::move(options)), current_(current), select_(std::move(select)) {}

  void onEnter() override {
    for (int i = 0; i < static_cast<int>(options_.size()); ++i) {
      fui::ListItem row;
      row.label = options_[i].c_str();
      row.actionValue = static_cast<int16_t>(i);
      row.value = i == current_ ? tr(STR_SELECTED) : nullptr;
      rows_.push_back(row);
    }
    UiListActivity::onEnter();
    moveSelectionTo(std::max(0, std::min(current_, listCount() - 1)));
  }

 private:
  std::string title_;
  std::vector<std::string> options_;
  std::vector<fui::ListItem> rows_;
  int current_;
  std::function<void(int)> select_;
  int listCount() const override { return static_cast<int>(options_.size()); }
  const char* headerTitle() const override { return title_.c_str(); }
  void activateIndex(int index) override {
    if (index < 0 || index >= listCount()) return;
    app.clearTapFlash();
    select_(index);
    finish();
  }
  void buildScreen(UiScreen& screen) override {
    const auto& metrics = UITheme::getInstance().getMetrics();
    screen.setContentMarginFromScreen(fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                                                  static_cast<int16_t>(metrics.buttonHintsHeight), 0});
    screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
    fui::ListProps props;
    props.items = rows_.data();
    props.count = static_cast<uint16_t>(rows_.size());
    props.action = ACTION_ROW;
    props.inputMask = fui::InputTouch;
    props.labelText = screen.theme().smallText;
    props.labelText.maxLines = 2;
    syncListViewport(screen, props);
    screen.list(props);
  }
};
}  // namespace

DictionarySettingsActivity::DictionarySettingsActivity(GfxRenderer& renderer, MappedInputManager& input)
    : UiListActivity("DictionarySettings", renderer, input) {}

void DictionarySettingsActivity::onEnter() {
  DICTIONARY_SETTINGS.ensureLoaded();
  sdFontSystem.refreshIfDirty();
  for (int i = 0; i < ITEM_COUNT; ++i) {
    rows_[i].label = i == Paragraphs ? paragraphLabel() : I18N.get(rowIds[i]);
    rows_[i].actionValue = static_cast<int16_t>(i);
  }
  UiListActivity::onEnter();
}

const char* DictionarySettingsActivity::headerTitle() const { return DictionarySettings::title(); }

void DictionarySettingsActivity::buildScreen(UiScreen& screen) {
  const auto& s = DICTIONARY_SETTINGS;
  const auto sizes = availableSizes();
  const StrId spacingIds[] = {StrId::STR_TIGHT, StrId::STR_NORMAL, StrId::STR_WIDE};
  values_[Font] = s.fontFamilyName();
  values_[Size] = std::to_string(snapToNearestPointSize(sizes, s.pointSize)) + " pt";
  values_[Margin] = std::to_string(s.margin) + " px";
  values_[Lines] = I18N.get(spacingIds[std::min<uint8_t>(s.lineSpacing, DictionarySettings::Wide)]);
  values_[Paragraphs] = s.paragraphSpacing ? tr(STR_NORMAL) : compactLabel();
  values_[Align] = s.alignment == DictionarySettings::Justify ? tr(STR_JUSTIFY) : tr(STR_ALIGN_LEFT);
  for (int i = 0; i < ITEM_COUNT; ++i) rows_[i].value = values_[i].c_str();

  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMarginFromScreen(fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                                                static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
  fui::ListProps props;
  props.items = rows_;
  props.count = ITEM_COUNT;
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  props.valueInset = 8;
  props.labelText = screen.theme().smallText;
  props.labelText.maxLines = 2;
  syncListViewport(screen, props);
  screen.list(props);
}

void DictionarySettingsActivity::activateIndex(int index) {
  if (index < 0 || index >= ITEM_COUNT) return;
  app.clearTapFlash();
  auto& s = DICTIONARY_SETTINGS;
  std::vector<std::string> options;
  std::function<void(int)> select;
  int current = 0;
  switch (index) {
    case Font: {
      options = {"Noto Sans", "Noto Serif"};
      std::vector<std::string> sdNames;
      current = s.family;
      for (const auto& family : sdFontSystem.registry().getFamilies()) {
        if (family.name.size() >= sizeof(s.sdFamily)) continue;
        if (family.name == s.sdFamily) current = static_cast<int>(options.size());
        options.push_back(family.name);
        sdNames.push_back(family.name);
      }
      select = [sdNames](int choice) {
        auto& settings = DICTIONARY_SETTINGS;
        if (choice < 2) {
          settings.family = static_cast<uint8_t>(choice);
          settings.sdFamily[0] = '\0';
        } else {
          const auto& name = sdNames[choice - 2];
          std::strncpy(settings.sdFamily, name.c_str(), sizeof(settings.sdFamily) - 1);
          settings.sdFamily[sizeof(settings.sdFamily) - 1] = '\0';
        }
        settings.pointSize = snapToNearestPointSize(availableSizes(), settings.pointSize);
      };
      break;
    }
    case Size: {
      const auto sizes = availableSizes();
      const uint8_t selected = snapToNearestPointSize(sizes, s.pointSize);
      for (const auto size : sizes) {
        if (size == selected) current = static_cast<int>(options.size());
        options.push_back(std::to_string(size) + " pt");
      }
      select = [sizes](int choice) { DICTIONARY_SETTINGS.pointSize = sizes[choice]; };
      break;
    }
    case Margin:
      for (int value = 5; value <= 20; ++value) options.push_back(std::to_string(value) + " px");
      current = s.margin - 5;
      select = [](int choice) { DICTIONARY_SETTINGS.margin = static_cast<uint8_t>(choice + 5); };
      break;
    case Lines:
      options = {tr(STR_TIGHT), tr(STR_NORMAL), tr(STR_WIDE)};
      current = s.lineSpacing;
      select = [](int choice) { DICTIONARY_SETTINGS.lineSpacing = static_cast<uint8_t>(choice); };
      break;
    case Paragraphs:
      options = {compactLabel(), tr(STR_NORMAL)};
      current = s.paragraphSpacing;
      select = [](int choice) { DICTIONARY_SETTINGS.paragraphSpacing = static_cast<uint8_t>(choice); };
      break;
    case Align:
      options = {tr(STR_ALIGN_LEFT), tr(STR_JUSTIFY)};
      current = s.alignment;
      select = [](int choice) { DICTIONARY_SETTINGS.alignment = static_cast<uint8_t>(choice); };
      break;
  }
  const char* title = index == Paragraphs ? paragraphLabel() : I18N.get(rowIds[index]);
  auto picker = makeUniqueNoThrow<DictionaryPickerActivity>(renderer, mappedInput, title, std::move(options), current,
      [select = std::move(select)](int choice) {
        select(choice);
        DICTIONARY_SETTINGS.saveToFile();
      });
  if (picker) startActivityForResult(std::move(picker), nullptr);
  else LOG_ERR("DICTSET", "Cannot allocate dictionary option picker");
}
