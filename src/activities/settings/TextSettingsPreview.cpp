#include "TextSettingsPreview.h"

#include <EpdFontFamily.h>
#include <Epub/ParsedText.h>
#include <Epub/blocks/BlockStyle.h>
#include <Epub/blocks/TextBlock.h>
#include <FontCacheManager.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <utility>

#include "CrossPointSettings.h"
#include "fontIds.h"
#include "customUiFontIds.h"
#include "TextSettingsPreviewGeometry.h"

namespace textsettings {

PreviewLayout::PreviewLayout() = default;
PreviewLayout::~PreviewLayout() = default;

namespace {

// Map the paragraph-alignment setting to the engine's CssTextAlign (BOOK_STYLE = justified)
CssTextAlign toCssAlign(uint8_t align) {
  if (align == CrossPointSettings::BOOK_STYLE) return CssTextAlign::Justify;
  return static_cast<CssTextAlign>(align);
}

// Lay the sample text out through the reader engine into layout.lines
void relayout(PreviewLayout& layout, const GfxRenderer& renderer, int fontId, int textWidth, const char* text) {
  layout.lines.clear();

  BlockStyle style;
  style.alignment = toCssAlign(SETTINGS.paragraphAlignment);
  style.textAlignDefined = true;  // honor the user's choice; RTL auto-detected from text

  ParsedText parsed(SETTINGS.hyphenationEnabled != 0, SETTINGS.focusReadingEnabled != 0, style,
                    SETTINGS.paragraphIndentSpaces);

  // Feed one space-separated word at a time; addWord handles NFC/CJK/RTL/focus splitting
  std::string word;
  for (const char* p = text;; p++) {
    if (*p == ' ' || *p == '\0') {
      if (!word.empty()) {
        parsed.addWord(word, EpdFontFamily::REGULAR);
        word.clear();
      }
      if (*p == '\0') break;
    } else {
      word.push_back(*p);
    }
  }

  parsed.layoutAndExtractLines(
      renderer, fontId, static_cast<uint16_t>(textWidth),
      [&layout](std::unique_ptr<TextBlock> line, uint32_t) { layout.lines.push_back(std::move(line)); }, true,
      SETTINGS.getCharacterSpacing(), SETTINGS.wordSpacing);
}

}  // namespace

int measurePreviewHeight(const GfxRenderer& renderer, PreviewLayout& layout, int previewPadding,
                         int minHeight, int maxHeight) {
  const int fontId = SETTINGS.getReaderFontId();
  const int textWidth = renderer.getScreenWidth() - 2 * (previewPadding + SETTINGS.screenMargin);
  if (fontId == 0 || textWidth <= 0 || maxHeight <= 0) {
    layout.lines.clear();
    layout.key = PreviewKey{};
    return std::clamp(minHeight, 0, std::max(0, maxHeight));
  }
  const float compression = SETTINGS.getReaderLineCompression();
  const int lineAdvance = std::max(1, renderer.getLineHeight(fontId, compression));
  // getTextHeight() is the ascender only. Reserve the normal line box plus
  // a small tail allowance so letters such as g/p cannot reach the label.
  const int lineH = std::max(renderer.getTextHeight(fontId), renderer.getLineHeight(fontId)) + 2;
  const int paragraphGap = SETTINGS.extraParagraphSpacing ? lineAdvance / 2 : 0;
  const int labelH = renderer.getLineHeight(UI_6_FONT_ID) + 2;
  const PreviewKey key{.fontId = fontId,
                       .fontPointSize = SETTINGS.fontPointSize,
                       .screenMargin = SETTINGS.screenMargin,
                       .textWidth = textWidth,
                       .maxPreviewHeight = maxHeight,
                       .lineCompression = compression,
                       .alignment = SETTINGS.paragraphAlignment,
                       .extraParagraphSpacing = SETTINGS.extraParagraphSpacing != 0,
                       .paragraphIndentSpaces = SETTINGS.paragraphIndentSpaces,
                       .characterSpacing = SETTINGS.getCharacterSpacing(),
                       .wordSpacingPercent = SETTINGS.wordSpacing,
                       .focusReading = SETTINGS.focusReadingEnabled != 0,
                       .hyphenation = SETTINGS.hyphenationEnabled != 0};
  if (key != layout.key) {
    std::string sample = I18N.get(StrId::STR_FONT_PREVIEW_TEXT);
    if (auto* fcm = renderer.getFontCacheManager()) {
      const std::string warmText = sample + " ...";
      fcm->prewarmCache(fontId, warmText.c_str(), SETTINGS.focusReadingEnabled ? 0x03 : 0x01);
    }
    relayout(layout, renderer, fontId, textWidth, sample.c_str());
    // Preserve at least the end of paragraph one and the start of paragraph
    // two. Shorten only the sample, at word boundaries (UTF-8 safe); keep
    // actual reader metrics, indentation, alignment and spacing unchanged.
    while (previewgeometry::boundaryHeight(static_cast<int>(layout.lines.size()), lineAdvance, lineH,
                                            paragraphGap, labelH) > maxHeight) {
      const auto space = sample.find_last_of(" ");
      if (space == std::string::npos) break;
      sample.resize(space);
      const std::string shortened = sample + "...";
      relayout(layout, renderer, fontId, textWidth, shortened.c_str());
    }
    layout.key = key;
  }
  return previewgeometry::chooseHeight(minHeight, maxHeight, static_cast<int>(layout.lines.size()),
                                        lineAdvance, lineH, paragraphGap, labelH);
}

void renderPreview(const GfxRenderer& renderer, PreviewLayout& layout, int previewPadding, int labelGap, int top,
                   int height, const char* familyName, const char* sizeName) {
  (void)labelGap;  // Compact fixed gap; horizontal theme padding is preserved.
  const int width = renderer.getScreenWidth() - 2 * previewPadding;
  if (width <= 0 || height <= 0) return;
  const int labelH = renderer.getLineHeight(UI_6_FONT_ID) + 2;
  const int labelY = top + height - previewgeometry::BOTTOM_PADDING - labelH;
  char labelBuf[128];
  snprintf(labelBuf, sizeof(labelBuf), "%s \"%s, %s\"", tr(STR_PREVIEW), familyName, sizeName);
  const std::string label = renderer.truncatedText(UI_6_FONT_ID, labelBuf, width);
  if (labelY >= top) renderer.drawText(UI_6_FONT_ID, previewPadding, labelY, label.c_str());

  const int fontId = SETTINGS.getReaderFontId();
  if (fontId == 0 || layout.lines.empty()) return;
  const int lineH = std::max(renderer.getTextHeight(fontId), renderer.getLineHeight(fontId)) + 2;
  const int lineAdvance = std::max(1, renderer.getLineHeight(fontId, SETTINGS.getReaderLineCompression()));
  const int paragraphGap = SETTINGS.extraParagraphSpacing ? lineAdvance / 2 : 0;
  const int textBottomLimit = labelY - previewgeometry::LABEL_GAP;
  int y = top + previewgeometry::TOP_PADDING;
  for (int paragraph = 0; paragraph < 2; paragraph++) {
    for (const auto& line : layout.lines) {
      if (y + lineH > textBottomLimit) return;
      line->render(renderer, fontId, previewPadding + SETTINGS.screenMargin, y);
      y += lineAdvance;
    }
    y += paragraphGap;
  }
}

}  // namespace textsettings
