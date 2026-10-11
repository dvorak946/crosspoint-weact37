#include "DictionaryDefinitionActivity.h"

#include <FontCacheManager.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>

#include "util/DictionarySettings.h"
#include "HapticFeedback.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/DictHtmlPages.h"
#include "util/HtmlToPlainText.h"

namespace {

// Longest measurable/drawable span. Wrapped lines stay under the screen width
// (far below this); only pathological unbreakable tokens are split at this cap.
constexpr size_t MAX_LINE_BYTES = 191;


// Styled-path ceiling: the laid-out Pages keep the whole definition resident
// (TextBlock arenas ≈ text + ~7 bytes/word plus per-line objects), roughly
// doubling the string's footprint while this activity is stacked over the
// reader and word-select. Bigger definitions take the span-based plain-text
// path, which holds no per-page copies.
constexpr size_t MAX_STYLED_HTML_BYTES = 16 * 1024;

}  // namespace

void DictionaryDefinitionActivity::onEnter() {
  Activity::onEnter();
  RenderLock lock;
  DICTIONARY_SETTINGS.ensureLoaded();
  bodyFontId = dictionaryFont.load(renderer, DICTIONARY_SETTINGS);
  // Normalize StarDict multi-type separators so the wrap loop and the
  // C-string font APIs below both see the whole definition.
  std::replace(definition.begin(), definition.end(), '\0', '\n');
  if (!(htmlDefinition && definition.size() <= MAX_STYLED_HTML_BYTES && layoutHtmlPages())) {
    definition = htmlToPlainText(definition);
    wrapText();
  }
  requestUpdate();
}

void DictionaryDefinitionActivity::onExit() {
  Activity::onExit();
  pages.clear();  // No retained layout can refer to a font after it is unregistered.
  dictionaryFont.unload(renderer);
  if (auto* fcm = renderer.getFontCacheManager()) {
    fcm->releaseSdFontCaches();
  }
}

int DictionaryDefinitionActivity::headerTextTop() const {
  return std::max(40, UITheme::getInstance().getMetrics().batteryBarHeight + 8);
}

int DictionaryDefinitionActivity::headerHeight() const {
  return std::max<int>(UITheme::getInstance().getMetrics().headerHeight,
                       headerTextTop() + renderer.getLineHeight(UI_10_FONT_ID) + 8);
}

DictionaryDefinitionActivity::BodyArea DictionaryDefinitionActivity::bodyArea() const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto orientation = renderer.getOrientation();
  const bool isLandscape = orientation == GfxRenderer::Orientation::LandscapeClockwise ||
                           orientation == GfxRenderer::Orientation::LandscapeCounterClockwise;
  const bool isInverted = orientation == GfxRenderer::Orientation::PortraitInverted;
  const int hintGutterWidth = isLandscape ? metrics.sideButtonHintsWidth : 0;
  const int topArea = (isInverted ? metrics.buttonHintsHeight : 0) + headerHeight();
  const int bottomArea = metrics.buttonHintsHeight + metrics.verticalSpacing;
  return {renderer.getScreenWidth() - hintGutterWidth - 2 * DICTIONARY_SETTINGS.margin,
          renderer.getScreenHeight() - topArea - bottomArea};
}

// Styled path: lay the HTML definition out through the EPUB chapter parser
// into reader-identical Pages. Frees `definition` on success (the page arenas
// own the text); any failure leaves state untouched for the plain-text path.
bool DictionaryDefinitionActivity::layoutHtmlPages() {
  const BodyArea body = bodyArea();
  if (body.width <= 0 || body.height <= 0) return false;
  const int tailPadding = std::max(0, renderer.getLineHeight(bodyFontId) -
                                        renderer.getLineHeight(bodyFontId, DICTIONARY_SETTINGS.lineCompression()));
  if (!buildDictionaryHtmlPages(renderer, definition, static_cast<uint16_t>(body.width),
                                static_cast<uint16_t>(std::max(1, body.height - tailPadding)), pages,
                                bodyFontId, DICTIONARY_SETTINGS)) {
    return false;
  }
  definition.clear();
  definition.shrink_to_fit();
  totalPages = static_cast<int>(pages.size());
  currentPage = 0;
  return true;
}

int DictionaryDefinitionActivity::measureSpan(const int fontId, const char* text, size_t len) const {
  char buf[MAX_LINE_BYTES + 1];
  len = std::min(len, MAX_LINE_BYTES);
  memcpy(buf, text, len);
  buf[len] = '\0';
  return renderer.getTextAdvanceX(fontId, buf, EpdFontFamily::REGULAR);
}

// Greedy word-wrap of `definition` into byte spans. '\n' breaks lines (blank
// lines survive as paragraph spacing; NULs from multi-type StarDict entries
// were normalized to newlines in onEnter); '\r' is dropped by treating it as
// a space at a token edge.
void DictionaryDefinitionActivity::wrapText() {
  lines.clear();
  lines.reserve(definition.size() / 32 + 8);

  const int fontId = bodyFontId;
  // SD-card fonts: merge every definition codepoint into the persistent
  // advance table up front. Otherwise each unseen codepoint measured below
  // falls back to an on-demand glyph load from SD (8-slot overflow ring).
  renderer.ensureSdCardFontReady(fontId, definition.c_str(), 0x01 /* REGULAR */);

  const BodyArea body = bodyArea();
  const int maxWidth = body.width;
  const int lineHeight = std::max(1, renderer.getLineHeight(fontId, DICTIONARY_SETTINGS.lineCompression()));
  const int fontHeight = std::max(lineHeight, renderer.getLineHeight(fontId));
  linesPerPage = std::max(1, 1 + (body.height - fontHeight) / lineHeight);

  const char* text = definition.c_str();
  const uint32_t n = static_cast<uint32_t>(definition.size());
  uint32_t lineStart = 0;
  uint32_t lineEnd = 0;  // one past the last token byte on the current line
  int lineWidth = 0;

  const auto flushLine = [&](uint32_t nextStart, bool paragraphEnd = false) {
    // Treat explicit newlines as paragraph boundaries. Normal adds a single
    // spacer; repeated blank source lines never multiply that gap.
    const auto len = static_cast<uint16_t>(lineEnd - lineStart);
    if (len != 0) {
      lines.push_back({lineStart, len, paragraphEnd});
      if (paragraphEnd && DICTIONARY_SETTINGS.paragraphSpacing) {
        lines.push_back({nextStart, 0, true});
      }
    }
    lineStart = nextStart;
    lineEnd = nextStart;
    lineWidth = 0;
  };

  uint32_t i = 0;
  while (i < n) {
    const char c = text[i];
    if (c == '\n' || c == '\0') {
      flushLine(i + 1, true);
      i++;
      continue;
    }
    if (c == ' ' || c == '\t' || c == '\r') {
      i++;
      continue;
    }

    // Token: run of non-whitespace bytes, capped at the measure buffer.
    const uint32_t tokenStart = i;
    while (i < n && text[i] != ' ' && text[i] != '\t' && text[i] != '\r' && text[i] != '\n' && text[i] != '\0' &&
           i - tokenStart < MAX_LINE_BYTES) {
      i++;
    }
    // If the byte cap cut the token mid-UTF-8-sequence, back off to the last
    // complete codepoint so measure/draw never see a partial sequence. A
    // natural stop lands on whitespace or the terminating NUL, never on a
    // continuation byte, so this is a no-op there.
    while (i - tokenStart > 1 && (text[i] & 0xC0) == 0x80) i--;
    const uint32_t tokenLen = i - tokenStart;
    const int tokenWidth = measureSpan(fontId, text + tokenStart, tokenLen);

    if (lineEnd == lineStart) {
      lineStart = tokenStart;
      lineEnd = tokenStart + tokenLen;
      lineWidth = tokenWidth;
    } else if (tokenStart + tokenLen - lineStart <= MAX_LINE_BYTES &&
               measureSpan(fontId, text + lineStart, tokenStart + tokenLen - lineStart) <= maxWidth) {
      lineEnd = tokenStart + tokenLen;
      lineWidth = measureSpan(fontId, text + lineStart, lineEnd - lineStart);
    } else {
      flushLine(tokenStart);
      lineEnd = tokenStart + tokenLen;
      lineWidth = tokenWidth;
    }

    // An unbreakable token wider than the screen is now alone on the line
    // (any previous content was flushed above): split it at the widest
    // fitting UTF-8 boundary and carry the remainder forward.
    while (lineWidth > maxWidth && lineEnd - lineStart > 1) {
      const uint32_t len = lineEnd - lineStart;
      uint32_t lastFit = 0;
      for (uint32_t f = 1; f <= len; f++) {
        if (f == len || (text[lineStart + f] & 0xC0) != 0x80) {  // codepoint boundary
          if (measureSpan(fontId, text + lineStart, f) > maxWidth) break;
          lastFit = f;
        }
      }
      if (lastFit == 0) {
        // Even a single over-wide glyph must make progress; consume its whole
        // UTF-8 sequence rather than splitting it into invalid fragments.
        lastFit = 1;
        while (lastFit < len && (text[lineStart + lastFit] & 0xC0) == 0x80) lastFit++;
      }
      const uint32_t rest = lineStart + lastFit;
      lineEnd = rest;
      flushLine(rest);
      lineEnd = rest + (len - lastFit);
      lineWidth = measureSpan(fontId, text + lineStart, lineEnd - lineStart);
    }
  }
  if (lineEnd > lineStart) flushLine(n, true);

  // Trim trailing blank lines so the last page is not empty padding.
  while (!lines.empty() && lines.back().len == 0) lines.pop_back();

  totalPages = std::max(1, (static_cast<int>(lines.size()) + linesPerPage - 1) / linesPerPage);
  currentPage = 0;
}

void DictionaryDefinitionActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  // Same tap zones as the reader page turns: left third = previous page,
  // the rest = next. Back is the usual left-edge swipe.
  int tx = 0;
  int ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    if (tx < renderer.getScreenWidth() / 3) {
      if (currentPage > 0) {
        haptic_feedback::touchAction();
        currentPage--;
        requestUpdate();
      }
    } else if (currentPage + 1 < totalPages) {
      haptic_feedback::touchAction();
      currentPage++;
      requestUpdate();
    }
    return;
  }

  // Nav1 is Down and Nav2 is Up on the user's swapped GPIO wiring.
  // Keep these dictionary controls stable when the screen is rotated.
  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Down, MappedInputManager::Button::Right}, [this] {
    if (currentPage + 1 < totalPages) {
      currentPage++;
      requestUpdate();
    }
  });

  buttonNavigator.onPressAndContinuous({MappedInputManager::Button::Up, MappedInputManager::Button::Left}, [this] {
    if (currentPage > 0) {
      currentPage--;
      requestUpdate();
    }
  });
}

// Draws the current page: a styled Page when the HTML layout succeeded,
// otherwise the wrapped line spans (copied into a stack buffer for NUL
// termination). Called twice per render: once in font-cache scan mode, once
// for the real paint.
void DictionaryDefinitionActivity::drawBody(const int fontId, const int x, const int startY) const {
  if (!pages.empty()) {
    pages[currentPage]->render(renderer, fontId, x, startY);
    return;
  }
  const int lineHeight = std::max(1, renderer.getLineHeight(fontId, DICTIONARY_SETTINGS.lineCompression()));
  const int firstLine = currentPage * linesPerPage;
  const int lastLine = std::min(firstLine + linesPerPage, static_cast<int>(lines.size()));
  for (int i = firstLine; i < lastLine; i++) {
    if (lines[i].len == 0) continue;
    drawPlainLine(fontId, x, startY + (i - firstLine) * lineHeight, lines[i]);
  }
}

void DictionaryDefinitionActivity::drawPlainLine(int fontId, int x, int y, const Line& line) const {
  char buf[MAX_LINE_BYTES + 1];
  const size_t len = std::min(static_cast<size_t>(line.len), MAX_LINE_BYTES);
  memcpy(buf, definition.c_str() + line.start, len);
  buf[len] = '\0';
  if (DICTIONARY_SETTINGS.alignment != DictionarySettings::Justify || line.paragraphEnd) {
    renderer.drawText(fontId, x, y, buf);
    return;
  }
  // At most 96 one-byte words in a 191-byte line. Stack spans avoid copies
  // per page; never stretch a paragraph's final line or a single long token.
  struct Word { uint16_t start, len, width; };
  Word words[96];
  size_t count = 0, cursor = 0;
  int wordWidth = 0;
  while (cursor < len && count < std::size(words)) {
    while (cursor < len && (buf[cursor] == ' ' || buf[cursor] == '\t' || buf[cursor] == '\r')) ++cursor;
    const size_t begin = cursor;
    while (cursor < len && buf[cursor] != ' ' && buf[cursor] != '\t' && buf[cursor] != '\r') ++cursor;
    if (cursor == begin) break;
    const int width = measureSpan(fontId, buf + begin, cursor - begin);
    words[count++] = {static_cast<uint16_t>(begin), static_cast<uint16_t>(cursor - begin), static_cast<uint16_t>(width)};
    wordWidth += width;
  }
  const int gap = renderer.getSpaceWidth(fontId, EpdFontFamily::REGULAR);
  const int extra = bodyArea().width - wordWidth - static_cast<int>(count > 0 ? count - 1 : 0) * gap;
  if (count < 2 || extra < 0) {
    renderer.drawText(fontId, x, y, buf);
    return;
  }
  int advance = 0;
  char word[MAX_LINE_BYTES + 1];
  for (size_t i = 0; i < count; ++i) {
    memcpy(word, buf + words[i].start, words[i].len);
    word[words[i].len] = '\0';
    renderer.drawText(fontId, x + advance + static_cast<int>(i * extra / (count - 1)), y, word);
    advance += words[i].width + gap;
  }
}

void DictionaryDefinitionActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto orientation = renderer.getOrientation();
  const bool isLandscapeCw = orientation == GfxRenderer::Orientation::LandscapeClockwise;
  const bool isLandscapeCcw = orientation == GfxRenderer::Orientation::LandscapeCounterClockwise;
  const bool isInverted = orientation == GfxRenderer::Orientation::PortraitInverted;
  const int hintGutterWidth = (isLandscapeCw || isLandscapeCcw) ? metrics.sideButtonHintsWidth : 0;
  const int contentX = isLandscapeCw ? hintGutterWidth : 0;
  const int contentWidth = renderer.getScreenWidth() - hintGutterWidth;
  const int contentY = isInverted ? metrics.buttonHintsHeight : 0;

  // Status-only themed header: clock and battery retain their usual positions.
  GUI.drawHeader(renderer, Rect{contentX, contentY, contentWidth, headerHeight()}, nullptr, nullptr, false);

  // Compact fixed UI font for the headword; body font remains independent.
  const int margin = DICTIONARY_SETTINGS.margin;
  const int headerY = contentY + headerTextTop();
  char counter[16] = {};
  if (totalPages > 1) snprintf(counter, sizeof(counter), "%d/%d", currentPage + 1, totalPages);
  const int counterWidth = renderer.getTextWidth(SMALL_FONT_ID, counter);
  const int reserved = counterWidth ? counterWidth + 8 : 0;
  const std::string title = renderer.truncatedText(UI_10_FONT_ID, headword.c_str(),
                                                  contentWidth - 2 * margin - reserved, EpdFontFamily::BOLD);
  renderer.drawText(UI_10_FONT_ID, contentX + margin, headerY, title.c_str(), true, EpdFontFamily::BOLD);
  const int counterY = headerY + renderer.getTextHeight(UI_10_FONT_ID) - renderer.getTextHeight(SMALL_FONT_ID);
  if (counterWidth) renderer.drawText(SMALL_FONT_ID, contentX + contentWidth - margin - counterWidth, counterY, counter);

  // Body: two-pass draw inside a prewarm scope (same pattern as the reader's
  // renderContents) so SD-card font glyphs load from SD in one batch instead
  // of one on-demand overflow read per character on every page turn.
  const int fontId = bodyFontId;
  const int bodyStartY = contentY + headerHeight();
  auto* fcm = renderer.getFontCacheManager();
  auto scope = fcm->createPrewarmScope();
  const BodyArea body = bodyArea();
  const auto clip = renderer.getClipRect();
  renderer.setClipRect(contentX + margin, bodyStartY, body.width, body.height);
  drawBody(fontId, contentX + margin, bodyStartY);  // scan pass: records codepoints only
  scope.endScanAndPrewarm();
  drawBody(fontId, contentX + margin, bodyStartY);
  renderer.setClipRect(clip[0], clip[1], clip[2], clip[3]);

  const char* previousLabel = currentPage > 0 ? "<" : "";
  const char* nextLabel = currentPage + 1 < totalPages ? ">" : "";
  const bool swapped = mappedInput.isNavDirectionSwapped();
  const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", swapped ? nextLabel : previousLabel,
                                            swapped ? previousLabel : nextLabel);
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
