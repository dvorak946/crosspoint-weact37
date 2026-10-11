#pragma once
#include <algorithm>

namespace textsettings::previewgeometry {
inline constexpr int TOP_PADDING = 2;
inline constexpr int BOTTOM_PADDING = 2;
inline constexpr int LABEL_GAP = 2;
// Entire first paragraph followed by the first line of the second paragraph.
inline int boundaryHeight(int lines, int advance, int lineBox, int paragraphGap, int labelBox) {
  if (lines <= 0) return TOP_PADDING + LABEL_GAP + labelBox + BOTTOM_PADDING;
  return TOP_PADDING + lines * advance + paragraphGap + lineBox + LABEL_GAP + labelBox + BOTTOM_PADDING;
}
inline int chooseHeight(int minimum, int maximum, int lines, int advance, int lineBox, int paragraphGap, int labelBox) {
  return std::clamp(std::max(minimum, boundaryHeight(lines, advance, lineBox, paragraphGap, labelBox)),
                    0, std::max(0, maximum));
}
}  // namespace textsettings::previewgeometry
