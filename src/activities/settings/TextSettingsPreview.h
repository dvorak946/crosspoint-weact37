#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class GfxRenderer;
class TextBlock;

namespace textsettings {

// Settings + geometry that determine the laid-out lines; used to invalidate the cache.
struct PreviewKey {
  int fontId = -1;
  int fontPointSize = -1;
  int screenMargin = -1;
  int textWidth = -1;
  int maxPreviewHeight = -1;
  float lineCompression = -1.0f;
  uint8_t alignment = 0xFF;
  bool extraParagraphSpacing = false;
  uint8_t paragraphIndentSpaces = 2;
  int8_t characterSpacing = 0;
  uint8_t wordSpacingPercent = 100;
  bool focusReading = false;
  bool hyphenation = false;
  bool operator==(const PreviewKey&) const = default;
};

// Cached engine preview lines + the key that produced them
struct PreviewLayout {
  PreviewLayout();
  ~PreviewLayout();

  std::vector<std::unique_ptr<TextBlock>> lines;
  PreviewKey key;
};

// Measures with the reader engine; may shorten the sample on compact screens
// to keep the paragraph boundary visible without changing font or spacing.
int measurePreviewHeight(const GfxRenderer& renderer, PreviewLayout& layout, int previewPadding,
                         int minHeight, int maxHeight);

// Draws the measured sample-text pane via the reader engine.
void renderPreview(const GfxRenderer& renderer, PreviewLayout& layout, int previewPadding, int labelGap, int top,
                   int height, const char* familyName, const char* sizeName);

}  // namespace textsettings
