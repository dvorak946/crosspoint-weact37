#pragma once

#include "components/themes/lyra/LyraTheme.h"

class GfxRenderer;

namespace Lyra3CoversMetrics {
constexpr ThemeMetrics values = [] {
  ThemeMetrics v = LyraMetrics::values;
  // Inherit the tested Lyra status clearance and navigation hints.
  // Five 36px menu rows fit below this card on the 240x416 panel.
  v.homeCoverHeight = 60;
  v.homeCoverTileHeight = 124;
  v.menuRowHeight = 36;
  v.homeRecentBooksCount = 3;
  return v;
}();
}  // namespace Lyra3CoversMetrics

class Lyra3CoversTheme : public LyraTheme {
 public:
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           const int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
  int getMenuRowHeight(const GfxRenderer&) const override { return Lyra3CoversMetrics::values.menuRowHeight; }
  int homeCoverThumbHeight(const GfxRenderer& renderer) const override;
};
