#pragma once

#include <GfxRenderer.h>
#include <algorithm>
#include "RoundedRaffTheme.h"
#include "customUiFontIds.h"

// One shared geometry for Home drawing, cover cache sizing and hit targets.
// Four or five horizontal tabs occupy the same band, so plugins never shrink art.
namespace RoundedRaffHomeLayout {
constexpr int titleFont = UI_9_FONT_ID;
constexpr int menuFont = UI_8_FONT_ID;
constexpr int guideFont = UI_6_FONT_ID;
constexpr int bottomMargin = 2;
constexpr int menuToHintsGap = 8;
constexpr int bookToMenuGap = 2;
constexpr int metadataGap = 2;
constexpr int selectionToPercentGap = 5;
constexpr int percentHeight = 9;
constexpr int progressHeight = 5;
constexpr int textInset = 3;
constexpr int tabHeight = 25;
// Five equal slots need a smaller font and a one-pixel inset to keep Books whole.
inline int menuFontForCount(int count) { return count >= 5 ? UI_7_FONT_ID : UI_8_FONT_ID; }
inline int tabTextInsetForCount(int count) { return count >= 5 ? 1 : 2; }
inline int tabWidthSafetyForCount(int count) { return count >= 5 ? 0 : 1; }
constexpr int hintsGap = 4;
constexpr int hintsRadius = 4;

inline int hintsHeight() {
  return RoundedRaffMetrics::values.buttonHintsHeight - 6;
}
inline int selectionHeight(const GfxRenderer& r) {
  return 2 * std::max(r.getLineHeight(titleFont), r.getTextHeight(titleFont)) +
         std::max(15, r.getLineHeight(guideFont)) + 2 * metadataGap;
}
inline int metadataHeight(const GfxRenderer& r) {
  return selectionHeight(r) + selectionToPercentGap + percentHeight + metadataGap + progressHeight;
}
inline int menuTop(const GfxRenderer& r, int /* count */) {
  return r.getScreenHeight() - bottomMargin - hintsHeight() - menuToHintsGap - tabHeight;
}
inline int coverHeight(const GfxRenderer& r, int count) {
  const auto& m = RoundedRaffMetrics::values;
  const int available = std::min(m.homeCoverTileHeight, menuTop(r, count) - bookToMenuGap - m.homeTopPadding);
  return std::max(0, available - metadataHeight(r) - metadataGap);
}
inline int coverSelectionBottom(const GfxRenderer& r, int count) {
  return RoundedRaffMetrics::values.homeTopPadding + coverHeight(r, count) + metadataGap + selectionHeight(r);
}
}  // namespace RoundedRaffHomeLayout
