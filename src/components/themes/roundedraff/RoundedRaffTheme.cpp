#include "RoundedRaffTheme.h"
#include "RoundedRaffHomeLayout.h"

#include <GfxRenderer.h>
#include <HalGPIO.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "customUiFontIds.h"
#include "util/BookProgress.h"

namespace {

constexpr int kTitleFontId = RoundedRaffHomeLayout::titleFont;
constexpr int kHintBottomMargin = RoundedRaffHomeLayout::bottomMargin;
constexpr int kGuideFontId = RoundedRaffHomeLayout::guideFont;
constexpr int kBookToMenuGap = RoundedRaffHomeLayout::bookToMenuGap;
constexpr int kMetadataGap = RoundedRaffHomeLayout::metadataGap;
constexpr int kPercentHeight = RoundedRaffHomeLayout::percentHeight;
constexpr int kProgressHeight = RoundedRaffHomeLayout::progressHeight;

// Six-column, nine-row custom bitmap. Digits 0..9 and percent.
constexpr uint8_t kPercentGlyphs[11][9] = {
    {30,33,33,33,33,33,33,33,30}, {4,12,20,4,4,4,4,4,31},
    {30,33,1,1,2,4,8,16,63}, {62,1,1,1,30,1,1,1,62},
    {2,6,10,18,34,63,2,2,2}, {63,32,32,32,62,1,1,33,30},
    {14,16,32,32,62,33,33,33,30}, {63,1,2,2,4,4,8,8,8},
    {30,33,33,33,30,33,33,33,30}, {30,33,33,33,31,1,1,2,28},
    {48,49,50,4,8,16,19,35,3}};

void drawPercent6x9(const GfxRenderer& renderer, int x, int y, int percent, bool black) {
  const std::string text = std::to_string(std::clamp(percent, 0, 100)) + "%";
  for (const char c : text) {
    const int glyph = c == '%' ? 10 : c - '0';
    for (int row = 0; row < 9; ++row)
      for (int col = 0; col < 6; ++col)
        if (kPercentGlyphs[glyph][row] & (1U << (5 - col))) renderer.fillRect(x + col, y + row, 1, 1, black);
    x += 7;
  }
}

// Drop complete UTF-8 codepoints only; no ellipsis and no clipped half-glyph.
std::string fitTabLabel(const GfxRenderer& renderer, int fontId, std::string label, int maxWidth) {
  if (maxWidth <= 0) return {};
  while (!label.empty() && renderer.getTextWidth(fontId, label.c_str()) > maxWidth) {
    size_t last = label.size() - 1;
    while (last > 0 && (static_cast<unsigned char>(label[last]) & 0xc0) == 0x80) --last;
    label.erase(last);
  }
  return label;
}

}  // namespace

void RoundedRaffTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle,
                                  const bool backButton) const {
  pendingHomeCover_ = {};  // Discard any unconsumed callback from an older screen.
  // Home's short band is status-only. The book title belongs under the cover.
  if (rect.height < RoundedRaffMetrics::values.headerHeight) {
    BaseTheme::drawHeader(renderer, rect, nullptr, nullptr, false);
  } else {
    BaseTheme::drawHeader(renderer, rect, title, subtitle, backButton);
  }
}

void RoundedRaffTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                           const int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                           bool& bufferRestored, std::function<bool()> storeCoverBuffer) const {
  // This callback is consumed by drawButtonMenu immediately in the same Home
  // render; it never runs asynchronously or on a later frame.
  pendingHomeCover_ = [this, &renderer, rect, &recentBooks, selectorIndex, &coverRendered, &coverBufferStored,
                       bufferRestored, storeCoverBuffer](const int menuTop) {
    Rect available = rect;
    available.height = std::max(0, std::min(rect.height, menuTop - kBookToMenuGap - rect.y));
    drawHomeCover(renderer, available, recentBooks, selectorIndex, coverRendered, coverBufferStored,
                   bufferRestored, storeCoverBuffer);
  };
}

void RoundedRaffTheme::drawHomeCover(GfxRenderer& renderer, Rect rect,
                                    const std::vector<RecentBook>& recentBooks, int selectorIndex, bool& coverRendered,
                                    bool& coverBufferStored, bool bufferRestored,
                                    const std::function<bool()>& storeCoverBuffer) const {
  const auto& metrics = RoundedRaffMetrics::values;
  const int tileX = rect.x + metrics.contentSidePadding;
  const int tileWidth = rect.width - 2 * metrics.contentSidePadding;
  if (tileWidth <= 0 || rect.height <= 0) return;
  if (recentBooks.empty()) {
    coverRendered = false;
    coverBufferStored = false;
    renderer.fillRectDither(tileX, rect.y, tileWidth, rect.height, Color::LightGray);
    const auto lines = renderer.wrappedText(kTitleFontId, tr(STR_NO_OPEN_BOOK), tileWidth - 16, 2);
    const int step = renderer.getLineHeight(kTitleFontId);
    int y = rect.y + (rect.height - static_cast<int>(lines.size()) * step) / 2;
    for (const auto& line : lines) {
      renderer.drawText(kTitleFontId, tileX + 8, y, line.c_str());
      y += step;
    }
    return;
  }

  // Keep two title lines reserved, including the font's actual line spacing.
  const auto& book = recentBooks[0];
  // Reload on Home entry (no saved region yet) or when the featured book changes.
  // Otherwise menu movement redraws only pixels and does not reread progress.
  if (!coverBufferStored || progressBookPath_ != book.path) {
    homeBookProgress_ = loadBookProgress(book.path);
    progressBookPath_ = book.path;
  }
  const int titleStep = std::max(renderer.getLineHeight(kTitleFontId), renderer.getTextHeight(kTitleFontId));
  const int metadataHeight = RoundedRaffHomeLayout::metadataHeight(renderer);
  if (rect.height <= metadataHeight + kMetadataGap) return;
  const int coverHeight = rect.height - metadataHeight - kMetadataGap;
  const Rect slot{tileX, rect.y, tileWidth, coverHeight};
  if (cachedCoverWidth_ != tileWidth || cachedCoverHeight_ != coverHeight) {
    coverRendered = false;
    coverBufferStored = false;
    cachedCoverWidth_ = tileWidth;
    cachedCoverHeight_ = coverHeight;
  }
  if (!bufferRestored) coverRendered = false;
  if (!coverRendered) {
    renderer.fillRect(slot.x, slot.y, slot.width, slot.height, false);
    bool hasCover = false;
    if (!book.coverBmpPath.empty()) {
      HalFile file;
      // Home supplies a width-sized, dithered BMP; clip its vertical center.
      const bool opened = Storage.openFileForRead("HOME", book.coverBmpPath, file);
      if (opened) {
        Bitmap bitmap(file);
        if (bitmap.parseHeaders() == BmpReaderError::Ok && bitmap.is1Bit() && bitmap.getWidth() == tileWidth && bitmap.getHeight() > 0) {
          // Draw cached pixels 1:1; never resize an already-dithered bitmap.
          hasCover = drawCoverThumbFill(renderer, bitmap, slot);
        }
        file.close();
      }
    }
    if (!hasCover) drawCoverPlaceholder(renderer, slot);
    coverBufferStored = storeCoverBuffer();
    // Failed covers are retried instead of becoming a permanently cached blank.
    coverRendered = coverBufferStored && hasCover;
  }
  const bool selected = selectorIndex == 0;
  renderer.drawRect(slot.x, slot.y, slot.width, slot.height, true);
  if (selected && slot.width > 2 && slot.height > 2)
    renderer.drawRect(slot.x + 1, slot.y + 1, slot.width - 2, slot.height - 2, true);
  const int titleTop = rect.y + coverHeight + kMetadataGap;
  const int barTop = rect.y + rect.height - kProgressHeight;
  const int percentTop = barTop - kMetadataGap - kPercentHeight;
  const int textX = tileX + RoundedRaffHomeLayout::textInset;
  const int textWidth = tileWidth - 2 * RoundedRaffHomeLayout::textInset;
  if (textWidth <= 0) return;
  // Selection ends above the fixed percentage band. Progress always remains
  // black on white, including while the book card is selected.
  renderer.fillRect(tileX, titleTop, tileWidth, barTop + kProgressHeight - titleTop, false);
  if (selected) renderer.fillRectDither(tileX, titleTop, tileWidth, RoundedRaffHomeLayout::selectionHeight(renderer),
                                       Color::LightGray);
  const auto lines = renderer.wrappedText(kTitleFontId, book.title.c_str(), textWidth, 2);
  for (size_t i = 0; i < lines.size(); ++i) {
    renderer.drawText(kTitleFontId, textX, titleTop + static_cast<int>(i) * titleStep, lines[i].c_str(), true);
  }
  if (!book.author.empty()) {
    const int authorTop = titleTop + static_cast<int>(lines.size()) * titleStep + kMetadataGap;
    const auto author = renderer.truncatedText(kGuideFontId, book.author.c_str(), textWidth);
    renderer.drawText(kGuideFontId, textX, authorTop, author.c_str(), true);
  }
  renderer.fillRoundedRect(textX, barTop, textWidth, kProgressHeight, 2, Color::LightGray);
  if (homeBookProgress_ >= 0) {
    const int percent = std::clamp(homeBookProgress_, 0, 100);
    const int fillWidth = textWidth * percent / 100;
    const int percentWidth = static_cast<int>(std::to_string(percent).size() + 1) * 7 - 1;
    const int percentX = std::clamp(textX + fillWidth - percentWidth / 2, textX,
                                  textX + std::max(0, textWidth - percentWidth));
    drawPercent6x9(renderer, percentX, percentTop, percent, true);
    if (fillWidth > 0) {
      const auto clip = renderer.getClipRect();
      const int left = std::max(textX, clip[0]);
      const int top = std::max(barTop, clip[1]);
      renderer.setClipRect(left, top, std::max(0, std::min(textX + fillWidth, clip[0] + clip[2]) - left),
                           std::max(0, std::min(barTop + kProgressHeight, clip[1] + clip[3]) - top));
      renderer.fillRoundedRect(textX, barTop, textWidth, kProgressHeight, 2, Color::Black);
      renderer.setClipRect(clip[0], clip[1], clip[2], clip[3]);
    }
  }
}

int RoundedRaffTheme::getMenuRowHeight(const GfxRenderer& renderer) const {
  (void)renderer;
  return RoundedRaffMetrics::values.menuRowHeight;
}

void RoundedRaffTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                      const std::function<std::string(int index)>& buttonLabel,
                                      const std::function<UIIcon(int index)>& rowIcon) const {
  (void)rowIcon;
  if (buttonCount <= 0) return;
  const auto& metrics = RoundedRaffMetrics::values;
  const int width = rect.width - 2 * metrics.contentSidePadding;
  if (width <= 0) return;
  const int menuY = RoundedRaffHomeLayout::menuTop(renderer, buttonCount);
  if (pendingHomeCover_) {
    auto drawCover = std::move(pendingHomeCover_);
    pendingHomeCover_ = {};
    drawCover(menuY);
  }

  const int fontId = RoundedRaffHomeLayout::menuFontForCount(buttonCount);
  const int textInset = RoundedRaffHomeLayout::tabTextInsetForCount(buttonCount);
  // A common font box gives every label the same baseline, including g/y.
  const auto& fonts = renderer.getFontMap();
  const auto font = fonts.find(fontId);
  const auto* data = font == fonts.end() ? nullptr : font->second.getData(EpdFontFamily::REGULAR);
  const int fontHeight = data ? data->ascender - std::min<int>(0, data->descender)
                              : renderer.getLineHeight(fontId);
  const int textY = menuY + std::max(0, (RoundedRaffHomeLayout::tabHeight - fontHeight) / 2);
  int x = rect.x + metrics.contentSidePadding;
  renderer.fillRectDither(x, menuY, width, RoundedRaffHomeLayout::tabHeight, Color::DarkGray);
  for (int i = 0; i < buttonCount; ++i) {
    const int slotWidth = width / buttonCount + (i < width % buttonCount ? 1 : 0);
    const int textLimit = slotWidth - 2 * textInset - RoundedRaffHomeLayout::tabWidthSafetyForCount(buttonCount);
    const std::string text = fitTabLabel(renderer, fontId, buttonLabel(i), textLimit);
    if (selectedIndex == i) renderer.fillRect(x, menuY, slotWidth, RoundedRaffHomeLayout::tabHeight, true);
    // Both selected and unselected text remain white. Only the slot background changes.
    if (!text.empty()) {
      const auto clip = renderer.getClipRect();
      const int left = std::max(x + textInset, clip[0]);
      const int top = std::max(menuY, clip[1]);
      renderer.setClipRect(left, top,
          std::max(0, std::min(x + slotWidth - textInset, clip[0] + clip[2]) - left),
          std::max(0, std::min(menuY + RoundedRaffHomeLayout::tabHeight, clip[1] + clip[3]) - top));
      const int textWidth = renderer.getTextWidth(fontId, text.c_str());
      renderer.drawText(fontId, x + (slotWidth - textWidth) / 2, textY, text.c_str(), false);
      renderer.setClipRect(clip[0], clip[1], clip[2], clip[3]);
    }
    x += slotWidth;
  }
}

void RoundedRaffTheme::drawTextField(const GfxRenderer& renderer, Rect rect, const int textWidth, bool cursorMode,
                                     int contentStartX, int contentWidth) const {
  BaseTheme::drawTextField(renderer, rect, textWidth, cursorMode, contentStartX, contentWidth);
}

void RoundedRaffTheme::drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                                       const char* btn4) const {
  if (gpio.hasTouch()) return;
  const auto origOrientation = renderer.getOrientation();
  renderer.setOrientation(GfxRenderer::Orientation::Portrait);
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const char* labels[] = {btn1, btn2, btn3, btn4};
  const auto present = [](const char* label) { return label != nullptr && label[0] != '\0'; };
  const bool groups[] = {present(btn1) || present(btn2), present(btn3) || present(btn4)};
  const int groupCount = static_cast<int>(groups[0]) + static_cast<int>(groups[1]);
  const int gap = RoundedRaffHomeLayout::hintsGap;
  const int height = std::min(pageHeight, RoundedRaffHomeLayout::hintsHeight());
  const int groupWidth = std::min(212, (pageWidth - 2 * RoundedRaffMetrics::values.contentSidePadding - gap * std::max(0, groupCount - 1)) / std::max(1, groupCount));
  const int hintY = pageHeight - height - kHintBottomMargin;
  const int totalWidth = groupCount * groupWidth + std::max(0, groupCount - 1) * gap;
  const bool grayscale = renderer.getRenderMode() != GfxRenderer::BW && !renderer.grayPlanesAreAbsolute();
  int visibleGroup = 0;
  for (int group = 0; group < 2; ++group) {
    if (!groups[group] || groupWidth <= 0 || height <= 0) continue;
    const int x = (pageWidth - totalWidth) / 2 + visibleGroup++ * (groupWidth + gap);
    renderer.fillRect(x, hintY, groupWidth, height, grayscale);
    if (grayscale) continue;
    renderer.drawRoundedRect(x, hintY, groupWidth, height, 2, RoundedRaffHomeLayout::hintsRadius, true);
    const int labelCount = static_cast<int>(present(labels[group * 2])) + static_cast<int>(present(labels[group * 2 + 1]));
    const int innerWidth = groupWidth - 8;
    int desired[2] = {};
    int desiredTotal = 0;
    for (int i = 0; i < 2; ++i) {
      const char* label = labels[group * 2 + i];
      if (present(label)) {
        desired[i] = renderer.getTextWidth(kGuideFontId, label) + 8;
        desiredTotal += desired[i];
      }
    }
    int cursorX = x + 4;
    int remaining = innerWidth;
    for (int i = 0; i < 2; ++i) {
      const char* label = labels[group * 2 + i];
      if (!present(label)) continue;
      // Resume takes its measured width; shorter labels receive less room.
      int cellWidth = desiredTotal <= innerWidth ? desired[i] + (innerWidth - desiredTotal) / labelCount
                                                : innerWidth * desired[i] / std::max(1, desiredTotal);
      if (i == 1 || !present(labels[group * 2 + 1])) cellWidth = remaining;
      cellWidth = std::min(remaining, cellWidth);
      drawHintLabel(renderer, kGuideFontId, label, cursorX, cellWidth, hintY, height);
      cursorX += cellWidth;
      remaining -= cellWidth;
    }
  }
  renderer.setOrientation(origOrientation);
}
