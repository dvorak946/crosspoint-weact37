#pragma once

#include <cstdint>

namespace CoverSizing {
// Match the image limits of the existing JPEG, PNG and BMP readers.
inline bool byWidth(int sourceWidth, int sourceHeight, int requestedWidth, int& width, int& height) {
  if (sourceWidth <= 0 || sourceHeight <= 0 || requestedWidth <= 0 || requestedWidth > 2048) return false;
  const int64_t scaledHeight = (int64_t(sourceHeight) * requestedWidth + sourceWidth / 2) / sourceWidth;
  if (scaledHeight > 3072) return false;
  width = requestedWidth;
  height = scaledHeight > 0 ? static_cast<int>(scaledHeight) : 1;
  return true;
}
}  // namespace CoverSizing
