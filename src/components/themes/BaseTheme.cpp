#include "BaseTheme.h"

#include <FreeInkUIGfxRenderer.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalGPIO.h>
#include <HalPowerManager.h>
#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>
#include <cstdint>
#include <string>

#include "I18n.h"
#include "RecentBooksStore.h"
#include "components/HeaderBackTapTarget.h"
#include "components/UIScale.h"
#include "components/UITheme.h"
#include "components/UIThemeTokens.h"
#include "components/UiAppHelpers.h"
#include "components/icons/bookmark.h"
#include "components/icons/cover.h"
#include "components/icons/headerIcons.h"
#include "fontIds.h"
#include "customUiFontIds.h" // IDs for the additional compact UI fonts.

freeink::ui::BitmapRef BaseTheme::checkboxIcon(const bool checked) {
  return freeink::ui::bitmapFromIcon(checked ? icon_checkbox_on_32 : icon_checkbox_off_32);
}

void BaseTheme::setCheckboxRow(freeink::ui::ListItem& item, const bool checked) {
  item.value = nullptr;
  item.toggle = true;
  item.toggleChecked = checked;
}

// Internal constants
namespace {
constexpr int bookmarkStatusIconWidth = 16;
constexpr int bookmarkStatusIconHeight = 14;
constexpr int bookmarkStatusIconGap = 4;
constexpr int bookmarkStatusIconTopCrop = 2;

// getTextHeight() reports only the ascender. Include the descender as well
// when a complete line must fit above a border or the progress bar.
int fontTextBoxHeight(const GfxRenderer& renderer, const int fontId) {
  const auto& fonts = renderer.getFontMap();
  const auto font = fonts.find(fontId);
  if (font == fonts.end()) return std::max(1, renderer.getLineHeight(fontId));
  const auto* data = font->second.getData(EpdFontFamily::REGULAR);
  return std::max(1, data->ascender + std::max(0, -data->descender));
}

void drawBookmarkStatusIcon(const GfxRenderer& renderer, const int x, const int y) {
  constexpr int bytesPerRow = bookmarkStatusIconWidth / 8;
  for (int row = 0; row < bookmarkStatusIconHeight; ++row) {
    for (int col = 0; col < bookmarkStatusIconWidth; ++col) {
      const uint8_t byte = BookmarkStatusIcon[(row + bookmarkStatusIconTopCrop) * bytesPerRow + col / 8];
      const uint8_t mask = 1U << (7 - (col % 8));
      renderer.drawPixel(x + col, y + row, (byte & mask) != 0);
    }
  }
}

}  // namespace

void BaseTheme::drawBatteryOutline(const GfxRenderer& renderer, int x, int y, int battWidth, int rectHeight) {
  // Top line
  renderer.drawLine(x + 1, y, x + battWidth - 3, y);
  // Bottom line
  renderer.drawLine(x + 1, y + rectHeight - 1, x + battWidth - 3, y + rectHeight - 1);
  // Left line
  renderer.drawLine(x, y + 1, x, y + rectHeight - 2);
  // Battery end
  renderer.drawLine(x + battWidth - 2, y + 1, x + battWidth - 2, y + rectHeight - 2);
  renderer.drawPixel(x + battWidth - 1, y + 3);
  renderer.drawPixel(x + battWidth - 1, y + rectHeight - 4);
  renderer.drawLine(x + battWidth - 0, y + 4, x + battWidth - 0, y + rectHeight - 5);
}

void BaseTheme::drawBatteryLightningBolt(const GfxRenderer& renderer, int boltX, int boltY) {
  // Draw lightning bolt (white/inverted on black fill for visibility)
  renderer.drawLine(boltX + 4, boltY + 0, boltX + 5, boltY + 0, false);
  renderer.drawLine(boltX + 3, boltY + 1, boltX + 4, boltY + 1, false);
  renderer.drawLine(boltX + 2, boltY + 2, boltX + 5, boltY + 2, false);
  renderer.drawLine(boltX + 3, boltY + 3, boltX + 4, boltY + 3, false);
  renderer.drawLine(boltX + 2, boltY + 4, boltX + 3, boltY + 4, false);
  renderer.drawLine(boltX + 1, boltY + 5, boltX + 4, boltY + 5, false);
  renderer.drawLine(boltX + 2, boltY + 6, boltX + 3, boltY + 6, false);
  renderer.drawLine(boltX + 1, boltY + 7, boltX + 2, boltY + 7, false);
}

void BaseTheme::fillBatteryIcon(const GfxRenderer& renderer, Rect rect, uint16_t percentage) const {
  const bool charging = gpio.isUsbConnected();

  const int maxFillWidth = rect.width - 5;
  const int fillHeight = rect.height - 4;
  if (maxFillWidth <= 0 || fillHeight <= 0) {
    return;
  }
  // +1 to round up so we always fill at least one pixel
  int filledWidth = percentage * maxFillWidth / 100 + 1;
  if (filledWidth > maxFillWidth) {
    filledWidth = maxFillWidth;
  }

  // When charging, ensure minimum fill so lightning bolt is fully visible
  constexpr int minFillForBolt = 8;
  if (charging && filledWidth < minFillForBolt) {
    filledWidth = std::min(minFillForBolt, maxFillWidth);
  }

  renderer.fillRect(rect.x + 2, rect.y + 2, filledWidth, fillHeight);

  if (charging) {
    drawBatteryLightningBolt(renderer, rect.x + 4, rect.y + 2);
  }
}

void BaseTheme::drawBatteryLeft(const GfxRenderer& renderer, Rect rect, const bool showPercentage,
                                const int fontId) const {
  const int textFontId = fontId != 0 ? fontId : SMALL_FONT_ID;  // Preserve other battery callers.
  // Left aligned: icon on left, percentage on right (reader mode)
  const uint16_t percentage = powerManager.getBatteryPercentage();
  // Selected status fonts can be shorter than the icon. Center the icon on
  // the text; legacy callers retain their original six-pixel offset.
  const int y = rect.y + (fontId != 0 ? (fontTextBoxHeight(renderer, textFontId) - rect.height) / 2 : 6);

  if (showPercentage) {
    const auto percentageText = std::to_string(percentage) + "%";
    renderer.drawText(textFontId, rect.x + batteryPercentSpacing + rect.width, rect.y, percentageText.c_str());
  }

  const Rect iconRect{rect.x, y, rect.width, rect.height};
  drawBatteryOutline(renderer, rect.x, y, rect.width, rect.height);
  fillBatteryIcon(renderer, iconRect, percentage);
}

void BaseTheme::drawCoverPlaceholder(const GfxRenderer& renderer, Rect rect) {
  if (rect.width <= 0 || rect.height <= 0) return;
  const int topHeight = rect.height / 3;
  renderer.fillRect(rect.x, rect.y, rect.width, rect.height, false);
  renderer.fillRect(rect.x, rect.y + topHeight, rect.width, rect.height - topHeight, true);
  renderer.drawRect(rect.x, rect.y, rect.width, rect.height, true);
  constexpr int ICON_SIZE = 32;
  if (rect.width >= ICON_SIZE + 4 && topHeight >= ICON_SIZE + 4) {
    const int insetX = std::min(24, (rect.width - ICON_SIZE) / 2);
    const int insetY = std::min(24, (topHeight - ICON_SIZE) / 2);
    renderer.drawIcon(CoverIcon, rect.x + insetX, rect.y + insetY, ICON_SIZE);
  }
}

bool BaseTheme::drawCoverThumbFill(const GfxRenderer& renderer, const Bitmap& bitmap, Rect slot, const int xOffset) {
  if (slot.width <= 0 || slot.height <= 0) return false;
  // xOffset nudges the centered art sideways; the clip stays on the slot.
  const int x = slot.x + (slot.width - bitmap.getWidth()) / 2 + xOffset;
  const int y = slot.y + (slot.height - bitmap.getHeight()) / 2;
  const auto clip = renderer.getClipRect();
  const int left = std::max(slot.x, clip[0]);
  const int top = std::max(slot.y, clip[1]);
  renderer.setClipRect(left, top, std::max(0, std::min(slot.x + slot.width, clip[0] + clip[2]) - left),
                       std::max(0, std::min(slot.y + slot.height, clip[1] + clip[3]) - top));
  const bool drawn = renderer.drawBitmap(bitmap, x, y, bitmap.getWidth(), bitmap.getHeight());
  renderer.setClipRect(clip[0], clip[1], clip[2], clip[3]);
  return drawn;
}

void BaseTheme::drawProgressBar(const GfxRenderer& renderer, Rect rect, const size_t current, const size_t total) {
  if (total == 0) {
    return;
  }

  // Use 64-bit arithmetic to avoid overflow for large files
  const int percent = static_cast<int>((static_cast<uint64_t>(current) * 100) / total);

  LOG_DBG("UI", "Drawing progress bar: current=%u, total=%u, percent=%d", current, total, percent);
  // Draw outline
  renderer.drawRect(rect.x, rect.y, rect.width, rect.height);

  // Draw filled portion
  const int fillWidth = (rect.width - 4) * percent / 100;
  if (fillWidth > 0) {
    renderer.fillRect(rect.x + 2, rect.y + 2, fillWidth, rect.height - 4);
  }

  // Draw percentage text centered below bar
  const std::string percentText = std::to_string(percent) + "%";
  renderer.drawCenteredText(UI_10_FONT_ID, rect.y + rect.height + 15, percentText.c_str());
}

// Centre hint labels by text height, wrapping to at most two lines.
void BaseTheme::drawHintLabel(const GfxRenderer& renderer, const int fontId, const char* label, const int x,
                              const int boxWidth, const int boxTop, const int boxHeight) {
  constexpr int textPadding = 4;  // keeps a wrapped label off the button's border
  const int maxTextWidth = boxWidth - (textPadding * 2);
  if (label == nullptr || maxTextWidth <= 0 || boxHeight <= 0) return;

  const int textHeight = fontTextBoxHeight(renderer, fontId);
  const int textWidth = renderer.getTextWidth(fontId, label);
  if (textWidth <= maxTextWidth) {
    const int textY = boxTop + std::max(1, (boxHeight - textHeight) / 2);
    renderer.drawText(fontId, x + (boxWidth - 1 - textWidth) / 2, textY, label);
    return;
  }

  // Spaced by the glyph height, not getLineHeight() — that returns the font's
  // full advanceY (leading included), which stacks two lines taller than the
  // button and clips the second one.
  constexpr int lineGap = 2;
  const int step = textHeight + lineGap;
  const int maxLines = std::min(2, std::max(1, (boxHeight - 2 + lineGap) / step));
  const auto lines = renderer.wrappedText(fontId, label, maxTextWidth, maxLines);
  const int block = static_cast<int>(lines.size()) * step - lineGap;
  int lineY = boxTop + std::max(1, (boxHeight - block) / 2);
  for (const auto& line : lines) {
    const int lineWidth = renderer.getTextWidth(fontId, line.c_str());
    renderer.drawText(fontId, x + (boxWidth - 1 - lineWidth) / 2, lineY, line.c_str());
    lineY += step;
  }
}

void BaseTheme::drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                                const char* btn4) const {
  if (gpio.hasTouch()) return;

  // Front-button guides are anchored to the physical portrait panel. Labels
  // were already mapped by MappedInputManager before this temporary rotation.
  const auto originalOrientation = renderer.getOrientation();
  renderer.setOrientation(GfxRenderer::Portrait);
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int buttonHeight = std::min(pageHeight, metrics.buttonHintsHeight);
  const int buttonTop = pageHeight - buttonHeight;
  const bool compact = pageWidth <= 320;
  const char* labels[] = {btn1, btn2, btn3, btn4};
#if defined(FREEINK_WEACT37_DRIVER) && FREEINK_WEACT37_DRIVER
  // Our WeAct board has Back/Confirm front pins (38/39), while front
  // Left/Right are PIN_UNASSIGNED. Up/Down are separate side-button roles.
  labels[2] = nullptr;
  labels[3] = nullptr;
#endif
  const auto present = [](const char* label) { return label != nullptr && label[0] != '\0'; };
  // Keep each front role in its own slot even when its current action is hidden.
  const int slotCount = present(labels[2]) || present(labels[3]) ? 4 : 2;
  constexpr int gap = 4;
  const int buttonWidth = compact ? std::min(106, (pageWidth - 8 - gap * (slotCount - 1)) / slotCount) : 106;
  const int groupWidth = slotCount * buttonWidth + (slotCount - 1) * gap;
  const int startX = (pageWidth - groupWidth) / 2;
  constexpr int narrowButtonPositions[] = {25, 130, 245, 350};
  constexpr int wideButtonPositions[] = {38, 154, 268, 384};
  const int* positions = pageWidth >= 528 ? wideButtonPositions : narrowButtonPositions;
  const int guideFontId = compact ? UI_6_FONT_ID : UI_10_FONT_ID;
  const bool grayscale = renderer.getRenderMode() != GfxRenderer::BW && !renderer.grayPlanesAreAbsolute();
  for (int i = 0; i < slotCount; ++i) {
    if (!present(labels[i]) || buttonWidth <= 0 || buttonHeight <= 0) continue;
    const int x = compact ? startX + i * (buttonWidth + gap) : positions[i];
    renderer.fillRect(x, buttonTop, buttonWidth, buttonHeight, grayscale);
    if (grayscale) continue;
    renderer.drawRect(x, buttonTop, buttonWidth, buttonHeight);
    drawHintLabel(renderer, guideFontId, labels[i], x, buttonWidth, buttonTop, buttonHeight);
  }
  renderer.setOrientation(originalOrientation);
}

void BaseTheme::drawSideButtonHints(const GfxRenderer& renderer, const char* topBtn, const char* bottomBtn) const {
  if (gpio.hasTouch()) {
    return;
  }

  const int screenWidth = renderer.getScreenWidth();
  const int screenHeight = renderer.getScreenHeight();
  const auto& metrics = UITheme::getInstance().getMetrics();
  const bool compact = std::min(screenWidth, screenHeight) <= 320;
  const int guideFontId = compact ? UI_6_FONT_ID : SMALL_FONT_ID;
  const int buttonWidth = compact ? std::min(20, metrics.sideButtonHintsWidth) : metrics.sideButtonHintsWidth;
  const int buttonHeight = compact ? 64 : 80;
  const int bottomLimit = screenHeight - metrics.buttonHintsHeight - 4;
  constexpr int buttonMargin = 4;

  if (gpio.hasEdgeSideButtons()) {
    // Edge-button layout (X3, X4 Pro): Up on left side, Down on right side, positioned higher
    const int x3ButtonY = std::max(0, std::min(screenHeight * 155 / 800, bottomLimit - buttonHeight));

    if (topBtn != nullptr && topBtn[0] != '\0') {
      const int leftX = buttonMargin;
      renderer.drawRect(leftX, x3ButtonY, buttonWidth, buttonHeight);
      const auto text = renderer.truncatedText(guideFontId, topBtn, buttonHeight - 8);
      const int textWidth = renderer.getTextWidth(guideFontId, text.c_str());
      const int textHeight = renderer.getTextHeight(guideFontId);
      const int textX = leftX + (buttonWidth - textHeight) / 2;
      const int textY = x3ButtonY + (buttonHeight + textWidth) / 2;
      renderer.drawTextRotated90CW(guideFontId, textX, textY, text.c_str());
    }

    if (bottomBtn != nullptr && bottomBtn[0] != '\0') {
      const int rightX = screenWidth - buttonMargin - buttonWidth;
      renderer.drawRect(rightX, x3ButtonY, buttonWidth, buttonHeight);
      const auto text = renderer.truncatedText(guideFontId, bottomBtn, buttonHeight - 8);
      const int textWidth = renderer.getTextWidth(guideFontId, text.c_str());
      const int textHeight = renderer.getTextHeight(guideFontId);
      const int textX = rightX + (buttonWidth - textHeight) / 2;
      const int textY = x3ButtonY + (buttonHeight + textWidth) / 2;
      renderer.drawTextRotated90CW(guideFontId, textX, textY, text.c_str());
    }
  } else {
    // X4 layout: Both buttons stacked on right side
    const int topButtonY = std::max(0, std::min(screenHeight * 345 / 800, bottomLimit - 2 * buttonHeight));
    const char* labels[] = {topBtn, bottomBtn};
    const int x = screenWidth - buttonMargin - buttonWidth;

    if (topBtn != nullptr && topBtn[0] != '\0') {
      renderer.drawLine(x, topButtonY, x + buttonWidth - 1, topButtonY);
      renderer.drawLine(x, topButtonY, x, topButtonY + buttonHeight - 1);
      renderer.drawLine(x + buttonWidth - 1, topButtonY, x + buttonWidth - 1, topButtonY + buttonHeight - 1);
    }

    if ((topBtn != nullptr && topBtn[0] != '\0') || (bottomBtn != nullptr && bottomBtn[0] != '\0')) {
      renderer.drawLine(x, topButtonY + buttonHeight, x + buttonWidth - 1, topButtonY + buttonHeight);
    }

    if (bottomBtn != nullptr && bottomBtn[0] != '\0') {
      renderer.drawLine(x, topButtonY + buttonHeight, x, topButtonY + 2 * buttonHeight - 1);
      renderer.drawLine(x + buttonWidth - 1, topButtonY + buttonHeight, x + buttonWidth - 1,
                        topButtonY + 2 * buttonHeight - 1);
      renderer.drawLine(x, topButtonY + 2 * buttonHeight - 1, x + buttonWidth - 1, topButtonY + 2 * buttonHeight - 1);
    }

    for (int i = 0; i < 2; i++) {
      if (labels[i] != nullptr && labels[i][0] != '\0') {
        const int y = topButtonY + i * buttonHeight;
        const auto text = renderer.truncatedText(guideFontId, labels[i], buttonHeight - 8);
        const int textWidth = renderer.getTextWidth(guideFontId, text.c_str());
        const int textHeight = renderer.getTextHeight(guideFontId);
        const int textX = x + (buttonWidth - textHeight) / 2;
        const int textY = y + (buttonHeight + textWidth) / 2;
        renderer.drawTextRotated90CW(guideFontId, textX, textY, text.c_str());
      }
    }
  }
}

// Slightly inside the side padding: the battery's boxed glyph and the clock
// digits read wider than text/cover ink on the same line, so flush placement
// looks like it overhangs the content columns.
int BaseTheme::headerStatusInset() { return UITheme::getInstance().getMetrics().headerSidePadding + 4; }

void BaseTheme::applyHeaderStatus(const GfxRenderer& renderer, freeink::ui::HeaderProps& props) {
  const ThemeMetrics& metrics = UITheme::getInstance().getMetrics();
  auto& status = props.status;

  // Status text stays at the fixed small font on every screen: FONT_LABEL is
  // bound to SMALL_FONT_ID by makeUiTarget() (FUI screens) and drawHeader()
  // (passive frames), while the uiScale FONT_SMALL is for list subtitles.
  status.battery.text.font = freeink::ui::GfxRendererTarget::FONT_LABEL;

  status.showBattery = true;
  status.battery.label = nullptr;
  status.clockText = nullptr;
  const uint16_t percentage = powerManager.getBatteryPercentage();
  status.battery.percent = static_cast<uint8_t>(percentage > 100 ? 100 : percentage);
  status.battery.charging = gpio.isUsbConnected();
  // Static label buffers: headers draw on the single render task, and the
  // strings only need to outlive the fui::header() call that consumes them.
  static char percentText[8];
  if (SETTINGS.hideBatteryPercentage != CrossPointSettings::HIDE_BATTERY_PERCENTAGE::HIDE_ALWAYS) {
    snprintf(percentText, sizeof(percentText), "%u%%", static_cast<unsigned>(percentage));
    status.battery.label = percentText;
  }
  status.battery.glyphWidth = static_cast<int16_t>(metrics.batteryWidth);
  status.battery.glyphHeight = static_cast<int16_t>(metrics.batteryHeight);
#if defined(FREEINK_WEACT37_DRIVER) && FREEINK_WEACT37_DRIVER
  // Compact only the header icon; keep the independently configured reader
  // status-bar battery and its lane-height reservation unchanged.
  status.battery.glyphWidth = 12;
  status.battery.glyphHeight = 8;
#endif
  status.battery.gap = batteryPercentSpacing;
  status.batteryLeft = metrics.headerBatterySide == 1;
  status.edgeInset = static_cast<int16_t>(headerStatusInset());

  // Header chrome geometry. Status lives on the theme's thin top strip in
  // fixed corners — battery top-right, a corner clock (headerClockCentered =
  // false) top-left, a centered clock top-center — and never repositions.
  // The content row (title, back arrow, trailing action buttons) centers on
  // the region between the strip and the band bottom, so its padding reads
  // as balanced under the status line rather than against the full band.
  // Text hangs low in its line cell by the font's internal leading; the icon
  // buttons drop by that amount to align with the glyphs the user sees.
  // Buttons keep a standard square (a band-8 button dwarfs its 24px icon);
  // the boxes are invisible and minTouchSize pads the tap target.
  const int16_t headerButtonSize = renderer.getScreenWidth() <= 320 ? 32 : 48;
  const int16_t bandHeight = static_cast<int16_t>(metrics.headerHeight);
  const int16_t strip = static_cast<int16_t>(metrics.batteryBarHeight);
  const int titleFontId = uiScaleSpec().titleFontId;
  const int16_t opticalDrop =
      static_cast<int16_t>((renderer.getLineHeight(titleFontId) - renderer.getTextHeight(titleFontId)) / 2);
  props.leadingSize = headerButtonSize;
  props.trailingSize = headerButtonSize;
  props.actionOffsetY = static_cast<int16_t>(
      std::min<int>(std::max<int>(0, bandHeight - headerButtonSize),
                    strip + std::max(0, (bandHeight - strip - headerButtonSize) / 2 - 4 + opticalDrop)));
  // Shift the title's band-centered box down by half the strip: its center
  // lands on the below-strip region's midline with the buttons.
  props.titleOffsetY = static_cast<int16_t>(strip / 2);
  status.stripHeight = strip;
  status.clockCentered = metrics.headerClockCentered;

  // Header clock, opposite the battery, on every screen that draws this band
  // (SETTINGS.clockShowInHeader). Themes whose title layout has no room for
  // the clock's left reserve opt out via headerShowsClock.
  static char clockText[10];
  if (metrics.headerShowsClock && SETTINGS.clockShowInHeader && halClock.isAvailable() &&
      halClock.formatTime(clockText, sizeof(clockText), SETTINGS.clockFormat == 1)) {
    status.clockText = clockText;
  }
}

void BaseTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle,
                           const bool backButton) const {
  // Every activity header renders through the FreeInkUI header + battery
  // indicator components, styled by the active theme's tokens (padding,
  // centering, underline). Non-interactive frame: no hit rects registered.
  namespace fui = freeink::ui;
  const auto spec = uiScaleSpec();
  fui::GfxRendererFrame<1> ui(renderer, spec.smallFontId, spec.bodyFontId, spec.titleFontId);
  // Refresh the app-wide shared tokens instead of copying ~1.5KB of
  // ThemeTokens onto this render-path stack frame; the values derived here
  // are identical to what every FreeInkApp screen derives. Goes through the
  // same publish-a-fresh-slot path applySharedUiTheme() uses (see
  // UiAppHelpers.h) rather than overwriting the previously-published
  // instance in place, since some other FreeInkApp could be mid-read of it.
  const fui::ThemeTokens& tokens = refreshSharedUiThemeTokens(ui.target);
  // Header status text (battery percent, right label) stays at the fixed
  // small font like the legacy headers; the uiScale small font is for list
  // subtitles.
  ui.target.setFont(fui::GfxRendererTarget::FONT_SMALL, SMALL_FONT_ID);
  ui.target.setFont(fui::GfxRendererTarget::FONT_LABEL, headerStatusFontId());
  // Callers pass a full-width header rect, so the battery/title would sit at the
  // true screen edges — clipped on rounded-corner panels (EEGO A4). Inset the band
  // horizontally by the board's viewable insets so header chrome clears the bezel.
  int hMarginTop, hMarginRight, hMarginBottom, hMarginLeft;
  renderer.getOrientedViewableTRBL(&hMarginTop, &hMarginRight, &hMarginBottom, &hMarginLeft);
  const fui::Rect band{static_cast<int16_t>(rect.x + hMarginLeft), static_cast<int16_t>(rect.y),
                       static_cast<int16_t>(rect.width - hMarginLeft - hMarginRight),
                       static_cast<int16_t>(rect.height)};

  fui::HeaderProps props;
  props.title = title;
  props.rightLabel = subtitle;  // firmware headers right-align the secondary text
  // Battery + clock chrome and their title reserves live in the FreeInkUI
  // header component; this only fills the values from settings and metrics.
  applyHeaderStatus(renderer, props);
  if (rect.height < UITheme::getInstance().getMetrics().headerHeight) {
    // Short bands (home) are not split into strip + content row: the title
    // centers on the band, clear of the band's bottom edge.
    props.titleOffsetY = 0;
  }
  // Tappable back button leading the band on touch boards, so every pushed
  // screen offers a visible way out beside the edge-swipe gesture. This frame
  // registers no hit rects, so the rect is recorded in HeaderBackTapTarget and
  // MappedInputManager folds taps on it into Button::Back. Same geometry as
  // the FUI header's leading slot (applyHeaderStatus set the size/offset) so
  // the recorded rect matches the drawn button.
  const int16_t backBtnSize = props.leadingSize;
  const bool showBackButton = backButton && title != nullptr && gpio.hasTouch();
  if (showBackButton) {
    props.leadingIcon = fui::bitmapFromIcon(icon_header_back_32);
    props.leadingAction = 1;  // any non-NO_ACTION id: paints the button, routing is via HeaderBackTapTarget
    HeaderBackTapTarget::set(band.x + 4, band.y + 4 + props.actionOffsetY, backBtnSize, backBtnSize);
  } else {
    HeaderBackTapTarget::clear();
  }
  props.borderEdges = fui::EdgeBottom;
  props.titleText = tokens.titleText;
  props.titleText.align = tokens.headerTitleAlign;
  props.subtitleText = tokens.smallText;
  props.styles = tokens.popup;
  props.sidePadding = tokens.headerSidePadding;
  // Underline only under a titled header: an untitled band (Lyra home screen)
  // historically drew no rule, and the old themes keyed the line on the title.
  if (title != nullptr && props.styles.normal.border.kind == fui::PaintKind::None && tokens.headerUnderline > 0) {
    props.styles.normal.border = fui::Paint::solid(fui::Color::Black);
    props.styles.normal.borderWidth = tokens.headerUnderline;
  }
  fui::header(ui.frame, band, props);
}

void BaseTheme::drawSubHeader(const GfxRenderer& renderer, Rect rect, const char* label, const char* rightLabel) const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  constexpr int labelGap = 6;
  const int contentWidth = std::max(0, rect.width - metrics.contentSidePadding * 2);

  int labelWidth = contentWidth;
  if (rightLabel) {
    auto truncatedRightLabel = renderer.truncatedText(SMALL_FONT_ID, rightLabel, contentWidth, EpdFontFamily::REGULAR);
    const int rightLabelWidth = renderer.getTextWidth(SMALL_FONT_ID, truncatedRightLabel.c_str());
    renderer.drawText(SMALL_FONT_ID, rect.x + rect.width - metrics.contentSidePadding - rightLabelWidth,
                      rect.y + 7, truncatedRightLabel.c_str());
    labelWidth = std::max(0, contentWidth - rightLabelWidth - labelGap);
  }

  if (labelWidth > 0) {
    auto truncatedLabel = renderer.truncatedText(UI_12_FONT_ID, label, labelWidth, EpdFontFamily::REGULAR);
    renderer.drawText(UI_12_FONT_ID, rect.x + metrics.contentSidePadding, rect.y, truncatedLabel.c_str(),
                      true, EpdFontFamily::REGULAR);
  }
}

// Draw the "Recent Book" cover card on the home screen
// TODO: Refactor method to make it cleaner, split into smaller methods
void BaseTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                    const int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                    bool& bufferRestored, std::function<bool()> storeCoverBuffer) const {
  if (rect.width <= 0 || rect.height <= 0) return;
  const auto& metrics = UITheme::getInstance().getMetrics();
  const bool compact = renderer.getScreenWidth() <= 320;
  const int titleFontId = compact ? UI_10_FONT_ID : UI_12_FONT_ID;
  const bool hasContinueReading = !recentBooks.empty();
  const bool bookSelected = hasContinueReading && selectorIndex == 0;

  // --- Top "book" card for the current title (selectorIndex == 0) ---
  // The caller reserves this slot from the compact home metrics.
  const int baseHeight = rect.height;

  int bookWidth = std::max(1, rect.width / 2), bookX;
  bool hasCoverImage = false;

  if (hasContinueReading && !recentBooks[0].coverBmpPath.empty()) {
    // Try to get actual image dimensions from BMP header
    const std::string coverBmpPath =
        UITheme::getCoverThumbPath(recentBooks[0].coverBmpPath, metrics.homeCoverHeight);

    HalFile file;
    if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        const int imgWidth = bitmap.getWidth();
        const int imgHeight = bitmap.getHeight();

        // Calculate width based on aspect ratio, maintaining baseHeight
        if (imgWidth > 0 && imgHeight > 0) {
          hasCoverImage = true;
          const float aspectRatio = static_cast<float>(imgWidth) / static_cast<float>(imgHeight);
          bookWidth = std::max(1, static_cast<int>(baseHeight * aspectRatio));

          // Ensure width doesn't exceed reasonable limits (max 90% of screen width)
          const int maxWidth = static_cast<int>(rect.width * 0.9f);
          if (bookWidth > maxWidth) {
            bookWidth = maxWidth;
          }
        } else {
          bookWidth = rect.width / 2;  // Fallback
        }
      }
    }
  }

  if (!hasCoverImage) {
    // No cover: use half screen size
    bookWidth = rect.width / 2;
  }

  bookX = rect.x + (rect.width - bookWidth) / 2;
  const int bookY = rect.y;
  const int bookHeight = baseHeight;

  // Bookmark dimensions (used in multiple places)
  const int bookmarkWidth = bookWidth / 8;
  const int bookmarkHeight = bookHeight / 5;
  const int bookmarkX = bookX + bookWidth - bookmarkWidth - 10;
  const int bookmarkY = bookY + 5;

  // Draw book card regardless, fill with message based on `hasContinueReading`
  {
    // Draw cover image as background if available (inside the box)
    // Only load from SD on first render, then use stored buffer

    if (hasContinueReading && !recentBooks[0].coverBmpPath.empty() && !coverRendered) {
      const std::string coverBmpPath =
          UITheme::getCoverThumbPath(recentBooks[0].coverBmpPath, metrics.homeCoverHeight);

      // First time: load cover from SD and render
      HalFile file;
      if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
        Bitmap bitmap(file);
        if (bitmap.parseHeaders() == BmpReaderError::Ok) {
          LOG_DBG("THEME", "Rendering bmp");

          // The card matches the cover aspect except when width-capped; fill
          // the card 1:1 and crop the overflow rather than rescale the dither.
          drawCoverThumbFill(renderer, bitmap, Rect{bookX, bookY, bookWidth, bookHeight});

          // Draw border around the card
          renderer.drawRect(bookX, bookY, bookWidth, bookHeight);

          // No bookmark ribbon when cover is shown - it would just cover the art

          // Store the buffer with cover image for fast navigation
          coverBufferStored = storeCoverBuffer();
          coverRendered = coverBufferStored;  // Only consider it rendered if we successfully stored the buffer

          // First render: if selected, draw selection indicators now
          if (bookSelected) {
            LOG_DBG("THEME", "Drawing selection");
            renderer.drawRect(bookX + 1, bookY + 1, bookWidth - 2, bookHeight - 2);
            renderer.drawRect(bookX + 2, bookY + 2, bookWidth - 4, bookHeight - 4);
          }
        }
      }
    }

    if (!bufferRestored && !coverRendered) {
      // No cover image: draw border or fill, plus bookmark as visual flair
      if (bookSelected) {
        renderer.fillRect(bookX, bookY, bookWidth, bookHeight);
      } else {
        renderer.drawRect(bookX, bookY, bookWidth, bookHeight);
      }

      // Draw bookmark ribbon when no cover image (visual decoration)
      if (hasContinueReading) {
        const int notchDepth = bookmarkHeight / 3;
        const int centerX = bookmarkX + bookmarkWidth / 2;

        const int xPoints[5] = {
            bookmarkX,                  // top-left
            bookmarkX + bookmarkWidth,  // top-right
            bookmarkX + bookmarkWidth,  // bottom-right
            centerX,                    // center notch point
            bookmarkX                   // bottom-left
        };
        const int yPoints[5] = {
            bookmarkY,                                // top-left
            bookmarkY,                                // top-right
            bookmarkY + bookmarkHeight,               // bottom-right
            bookmarkY + bookmarkHeight - notchDepth,  // center notch point
            bookmarkY + bookmarkHeight                // bottom-left
        };

        // Draw bookmark ribbon (inverted if selected)
        renderer.fillPolygon(xPoints, yPoints, 5, !bookSelected);
      }
    }

    // If buffer was restored, draw selection indicators if needed
    if (bufferRestored && bookSelected && coverRendered) {
      // Draw selection border (no bookmark inversion needed since cover has no bookmark)
      renderer.drawRect(bookX + 1, bookY + 1, bookWidth - 2, bookHeight - 2);
      renderer.drawRect(bookX + 2, bookY + 2, bookWidth - 4, bookHeight - 4);
    } else if (!coverRendered && !bufferRestored) {
      // Selection border already handled above in the no-cover case
    }
  }

  const int textPadding = compact ? std::min(10, bookWidth / 8) : 20;
  const int textSpace = std::max(1, bookWidth - 2 * textPadding);

  if (hasContinueReading) {
    const std::string& lastBookTitle = recentBooks[0].title;
    const std::string& lastBookAuthor = recentBooks[0].author;

    // Invert text colors based on selection state:
    // - With cover: selected = white text on black box, unselected = black text on white box
    // - Without cover: selected = white text on black card, unselected = black text on white card

    const int titleLineHeight = renderer.getLineHeight(titleFontId);
    const int authorHeight = lastBookAuthor.empty() ? 0 : renderer.getLineHeight(UI_10_FONT_ID) * 3 / 2;
    const int continueHeight = renderer.getLineHeight(UI_10_FONT_ID) * 3 / 2;
    const int titleLineLimit = compact ? std::min(2, std::max(1, (bookHeight - authorHeight - continueHeight - 16) /
                                                                  std::max(1, titleLineHeight))) : 3;
    auto lines = renderer.wrappedText(titleFontId, lastBookTitle.c_str(), textSpace, titleLineLimit);

    // Book title text
    int totalTextHeight = renderer.getLineHeight(titleFontId) * static_cast<int>(lines.size());
    if (!lastBookAuthor.empty()) {
      totalTextHeight += renderer.getLineHeight(UI_10_FONT_ID) * 3 / 2;
    }

    // Vertically center the title block within the card
    int titleYStart = bookY + std::max(4, (bookHeight - continueHeight - totalTextHeight) / 2);

    const auto truncatedAuthor = lastBookAuthor.empty()
                                     ? std::string{}
                                     : renderer.truncatedText(UI_10_FONT_ID, lastBookAuthor.c_str(), textSpace);

    // If cover image was rendered, draw box behind title and author
    if (coverRendered) {
      const int boxPadding = compact ? 4 : 8;
      // Calculate the max text width for the box
      int maxTextWidth = 0;
      for (const auto& line : lines) {
        const int lineWidth = renderer.getTextWidth(titleFontId, line.c_str());
        if (lineWidth > maxTextWidth) {
          maxTextWidth = lineWidth;
        }
      }
      if (!truncatedAuthor.empty()) {
        const int authorWidth = renderer.getTextWidth(UI_10_FONT_ID, truncatedAuthor.c_str());
        if (authorWidth > maxTextWidth) {
          maxTextWidth = authorWidth;
        }
      }

      const int boxWidth = maxTextWidth + boxPadding * 2;
      const int boxHeight = totalTextHeight + boxPadding * 2;
      const int boxX = rect.x + (rect.width - boxWidth) / 2;
      const int boxY = titleYStart - boxPadding;

      // Draw box (inverted when selected: black box instead of white)
      renderer.fillRect(boxX, boxY, boxWidth, boxHeight, bookSelected);
      // Draw border around the box (inverted when selected: white border instead of black)
      renderer.drawRect(boxX, boxY, boxWidth, boxHeight, !bookSelected);
    }

    for (const auto& line : lines) {
      renderer.drawText(titleFontId, bookX + (bookWidth - renderer.getTextWidth(titleFontId, line.c_str())) / 2,
                        titleYStart, line.c_str(), !bookSelected);
      titleYStart += renderer.getLineHeight(titleFontId);
    }

    if (!truncatedAuthor.empty()) {
      titleYStart += renderer.getLineHeight(UI_10_FONT_ID) / 2;
      renderer.drawText(UI_10_FONT_ID, bookX + (bookWidth - renderer.getTextWidth(UI_10_FONT_ID, truncatedAuthor.c_str())) / 2,
                        titleYStart, truncatedAuthor.c_str(), !bookSelected);
    }

    // "Continue Reading" label at the bottom
    const int continueY = bookY + bookHeight - renderer.getLineHeight(UI_10_FONT_ID) * 3 / 2;
    const auto continueLabel = renderer.truncatedText(UI_10_FONT_ID, tr(STR_CONTINUE_READING), std::max(1, bookWidth - 12));
    const int continueTextX = bookX + (bookWidth - renderer.getTextWidth(UI_10_FONT_ID, continueLabel.c_str())) / 2;
    if (coverRendered) {
      // Draw box behind "Continue Reading" text (inverted when selected: black box instead of white)
      const char* continueText = continueLabel.c_str();
      const int continueTextWidth = renderer.getTextWidth(UI_10_FONT_ID, continueText);
      constexpr int continuePadding = 6;
      const int continueBoxWidth = continueTextWidth + continuePadding * 2;
      const int continueBoxHeight = renderer.getLineHeight(UI_10_FONT_ID) + continuePadding;
      const int continueBoxX = rect.x + (rect.width - continueBoxWidth) / 2;
      const int continueBoxY = continueY - continuePadding / 2;
      renderer.fillRect(continueBoxX, continueBoxY, continueBoxWidth, continueBoxHeight, bookSelected);
      renderer.drawRect(continueBoxX, continueBoxY, continueBoxWidth, continueBoxHeight, !bookSelected);
      renderer.drawText(UI_10_FONT_ID, continueTextX, continueY, continueText, !bookSelected);
    } else {
      renderer.drawText(UI_10_FONT_ID, continueTextX, continueY, continueLabel.c_str(), !bookSelected);
    }
  } else {
    // No book to continue reading
    const int y =
        bookY + (bookHeight - renderer.getLineHeight(titleFontId) - renderer.getLineHeight(UI_10_FONT_ID)) / 2;
    const auto emptyTitle = renderer.truncatedText(titleFontId, tr(STR_NO_OPEN_BOOK), textSpace);
    const auto emptyHelp = renderer.truncatedText(UI_10_FONT_ID, tr(STR_START_READING), textSpace);
    renderer.drawText(titleFontId, bookX + (bookWidth - renderer.getTextWidth(titleFontId, emptyTitle.c_str())) / 2,
                      y, emptyTitle.c_str());
    renderer.drawText(UI_10_FONT_ID, bookX + (bookWidth - renderer.getTextWidth(UI_10_FONT_ID, emptyHelp.c_str())) / 2,
                      y + renderer.getLineHeight(titleFontId), emptyHelp.c_str());
  }
}

int BaseTheme::getMenuRowHeight(const GfxRenderer&) const { return UITheme::getInstance().getMetrics().menuRowHeight; }

void BaseTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                               const std::function<std::string(int index)>& buttonLabel,
                               const std::function<UIIcon(int index)>& rowIcon) const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  for (int i = 0; i < buttonCount; ++i) {
    const int tileY = metrics.verticalSpacing + rect.y +
                      static_cast<int>(i) * (metrics.menuRowHeight + metrics.menuSpacing);

    const bool selected = selectedIndex == i;

    if (selected) {
      renderer.fillRect(rect.x + metrics.contentSidePadding, tileY,
                        rect.width - metrics.contentSidePadding * 2, metrics.menuRowHeight);
    } else {
      renderer.drawRect(rect.x + metrics.contentSidePadding, tileY,
                        rect.width - metrics.contentSidePadding * 2, metrics.menuRowHeight);
    }

    std::string labelStr = buttonLabel(i);
    labelStr = renderer.truncatedText(UI_10_FONT_ID, labelStr.c_str(),
                                      std::max(1, rect.width - metrics.contentSidePadding * 2 - 8));
    const char* label = labelStr.c_str();
    const int textWidth = renderer.getTextWidth(UI_10_FONT_ID, label);
    const int textX = rect.x + (rect.width - textWidth) / 2;
    const int lineHeight = renderer.getLineHeight(UI_10_FONT_ID);
    const int textY =
        tileY + (metrics.menuRowHeight - lineHeight) / 2;  // vertically centered assuming y is top of text
    // Invert text when the tile is selected, to contrast with the filled background
    renderer.drawText(UI_10_FONT_ID, textX, textY, label, selectedIndex != i);
  }
}

Rect BaseTheme::drawPopup(const GfxRenderer& renderer, const char* message) const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int marginX = metrics.popupMarginX;
  const int fontId = renderer.getScreenWidth() <= 320 ? UI_10_FONT_ID : UI_12_FONT_ID;
  const int marginY = metrics.popupMarginY;
  const int frameThickness = metrics.popupFrameThickness;
  const EpdFontFamily::Style popupFontFamily = metrics.popupTextBold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  // Scale y position proportionally to screen height
  const int y = static_cast<int>(renderer.getScreenHeight() * metrics.popupTopOffsetRatio);
  const int availableWidth = std::max(1, renderer.getScreenWidth() - marginX * 2 - frameThickness * 2 - 8);
  const auto popupText = renderer.truncatedText(fontId, message, availableWidth, popupFontFamily);
  const int textWidth = renderer.getTextWidth(fontId, popupText.c_str(), popupFontFamily);
  const int textHeight = renderer.getLineHeight(fontId);
  const int w = textWidth + marginX * 2;
  const int h = textHeight + marginY * 2;
  const int x = (renderer.getScreenWidth() - w) / 2;

  const bool useRoundedPopup = metrics.popupCornerRadius > 0;
  if (useRoundedPopup) {
    renderer.fillRoundedRect(x - frameThickness, y - frameThickness, w + frameThickness * 2, h + frameThickness * 2,
                             metrics.popupCornerRadius + frameThickness, Color::White);
    renderer.fillRoundedRect(x, y, w, h, metrics.popupCornerRadius, Color::Black);
  } else {
    renderer.fillRect(x - frameThickness, y - frameThickness, w + frameThickness * 2, h + frameThickness * 2, true);
    renderer.fillRect(x, y, w, h, false);
  }

  const int textX = x + (w - textWidth) / 2;
  const int textY = y + marginY + metrics.popupTextBaselineOffsetY;
  renderer.drawText(fontId, textX, textY, popupText.c_str(), metrics.popupTextInverted, popupFontFamily);
  renderer.displayBuffer();
  return Rect{x, y, w, h};
}

void BaseTheme::fillPopupProgress(const GfxRenderer& renderer, const Rect& layout, const int progress) const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int barHeight = metrics.popupProgressBarHeight;
  const int barWidth =
      std::max(0, layout.width - metrics.popupMarginX * 2);  // twice the margin in drawPopup to match text width
  const int barX = layout.x + (layout.width - barWidth) / 2;
  const int barY = layout.y + layout.height - metrics.popupMarginY / 2 - barHeight / 2 - 1;
  if (barWidth <= 0 || barHeight <= 0) {
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  const int scaledProgress = metrics.popupProgressClampPercent ? std::clamp(progress, 0, 100) : progress;
  const int fillWidth = barWidth * scaledProgress / 100;

  if (metrics.popupProgressDrawOutline) {
    renderer.drawRect(barX, barY, barWidth, barHeight, 1, metrics.popupProgressOutlineInverted);
  }
  if (fillWidth > 0) {
    renderer.fillRect(barX, barY, fillWidth, barHeight, metrics.popupProgressFillInverted);
  }

  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}

int BaseTheme::getStatusBarTextLaneHeight() {
  // Full line boxes from the shipped regular font metadata:
  // Ubuntu 6: 12 + 3; Ubuntu 7: 14 + 3; Noto Sans 8: 18 + 5.
  // getTextHeight() alone returns just 12/14/18, losing the descender.
  const int fontId = SETTINGS.statusBarSpec().fontId;
  const int lineBox = fontId == UI_6_FONT_ID ? 15 : fontId == UI_7_FONT_ID ? 17 : 23;
  return lineBox + 2;
}

void BaseTheme::drawStatusBar(GfxRenderer& renderer, const float bookProgress, const int currentPage,
                              const int pageCount, std::string title, const int paddingBottom, const int textYOffset,
                              const bool fillMargin, const bool isPageBookmarked, const bool pageCountEstimated) {
  auto metrics = UITheme::getInstance().getMetrics();
  int orientedMarginTop, orientedMarginRight, orientedMarginBottom, orientedMarginLeft;
  renderer.getOrientedViewableTRBL(&orientedMarginTop, &orientedMarginRight, &orientedMarginBottom,
                                   &orientedMarginLeft);
  const auto sb = SETTINGS.statusBarSpec();
  const int statusFontId = sb.fontId;  // Measure and draw every status label with the selected font.
  const bool showStatusBarTextLane = sb.textLaneVisible(halClock.isAvailable());

  // Pin the text lane just above the progress bar, independently of the
  // reserved status-bar height and the chosen font's line spacing.
  const int statusTextHeight = fontTextBoxHeight(renderer, statusFontId);
  int laneHeight = statusTextHeight;
  if (sb.showBattery) laneHeight = std::max(laneHeight, metrics.batteryHeight);
  if (showStatusBarTextLane && isPageBookmarked) laneHeight = std::max(laneHeight, bookmarkStatusIconHeight);
  const int statusBottom = renderer.getScreenHeight() - orientedMarginBottom - paddingBottom;
  const int progressBarY = statusBottom - sb.progressBarHeightPx + (fillMargin ? 1 : 0);
  const int laneBottom = (sb.showsProgressBar() ? progressBarY : statusBottom) - 2;
  const int laneTop = laneBottom - laneHeight;
  auto textY = laneTop + (laneHeight - statusTextHeight) / 2;

  const int leftClusterX = metrics.statusBarHorizontalMargin + orientedMarginLeft + 1;
  const int rightClusterX = renderer.getScreenWidth() - metrics.statusBarHorizontalMargin - orientedMarginRight;
  int leftClusterWidth = 0;
  int rightClusterWidth = 0;

  if (sb.showBookProgressPercent || sb.showChapterPageCount) {
    // Right aligned text for progress counter
    char progressStr[32];

    // Use the same font for the estimate marker and the page counter.
    const bool showEstimate = pageCountEstimated && sb.showChapterPageCount;

    if (sb.showBookProgressPercent && sb.showChapterPageCount) {
      snprintf(progressStr, sizeof(progressStr), "%d/%d  %.0f%%", currentPage, pageCount, bookProgress);
    } else if (sb.showBookProgressPercent) {
      snprintf(progressStr, sizeof(progressStr), "%.0f%%", bookProgress);
    } else {
      snprintf(progressStr, sizeof(progressStr), "%d/%d", currentPage, pageCount);
    }

    int progressTextWidth = renderer.getTextWidth(statusFontId, progressStr);
    const int estimateWidth = showEstimate ? renderer.getTextWidth(statusFontId, "~") : 0;
    constexpr int estimateGap = 2;
    const int estimateSpacing = showEstimate ? estimateGap : 0;
    const int progressX = rightClusterX - estimateWidth - estimateSpacing - progressTextWidth;
    if (showEstimate) {
      renderer.drawText(statusFontId, progressX, textY, "~");
    }
    renderer.drawText(statusFontId, progressX + estimateWidth + estimateSpacing, textY, progressStr);

    rightClusterWidth += estimateWidth + estimateSpacing + progressTextWidth;
  }

  // Draw Progress Bar
  if (sb.showsProgressBar()) {
    const int barMarginLeft = fillMargin ? 0 : orientedMarginLeft;
    const int barMarginRight = fillMargin ? 0 : orientedMarginRight;
    const int progressBarMaxWidth = renderer.getScreenWidth() - barMarginLeft - barMarginRight;
    size_t progress;
    if (sb.progressBarMode == CrossPointSettings::STATUS_BAR_PROGRESS_BAR::BOOK_PROGRESS) {
      progress = static_cast<size_t>(bookProgress);
    } else {
      // Chapter progress
      progress = (pageCount > 0) ? (static_cast<float>(currentPage) / pageCount) * 100 : 0;
    }
    const int barWidth = progressBarMaxWidth * progress / 100;
    const int barHeight = sb.progressBarHeightPx + (fillMargin ? orientedMarginBottom - 1 : 0);
    renderer.fillRect(barMarginLeft, progressBarY, barWidth, barHeight, true);
  }

  // Draw Battery
  const bool showBatteryPercentage = sb.showBatteryPercent;

  if (sb.showBattery) {
    GUI.drawBatteryLeft(renderer,
                        Rect{leftClusterX + leftClusterWidth, textY, metrics.batteryWidth, metrics.batteryHeight},
                        showBatteryPercentage, statusFontId);
    int batteryWidth = metrics.batteryWidth;

    if (showBatteryPercentage) {
      const uint16_t percentage = powerManager.getBatteryPercentage();
      // width of icon + spacing + text for layout purposes
      batteryWidth +=
          batteryPercentSpacing + renderer.getTextWidth(statusFontId, (std::to_string(percentage) + "%").c_str());
    }

    leftClusterWidth += batteryWidth;
  }

  // Draw Clock (any board whose RTC probe succeeded)
  if (sb.showsClock() && halClock.isAvailable()) {
    char timeBuf[9];
    if (halClock.formatTime(timeBuf, sizeof(timeBuf), sb.clock12h)) {
      int clockTextWidth = renderer.getTextWidth(statusFontId, timeBuf);
      int clockX = 0;
      // Position to the left or right of the progress text (with a small gap).
      const int clockGap = renderer.getScreenWidth() <= 320 ? 4 : 10;
      if (sb.clockMode == CrossPointSettings::STATUS_BAR_CLOCK_LEFT) {
        clockX = leftClusterX + leftClusterWidth + (leftClusterWidth > 0 ? clockGap : 0);
        leftClusterWidth += clockTextWidth + (leftClusterWidth > 0 ? clockGap : 0);
      } else if (sb.clockMode == CrossPointSettings::STATUS_BAR_CLOCK_RIGHT) {
        clockX = rightClusterX - rightClusterWidth - (rightClusterWidth > 0 ? clockGap : 0) - clockTextWidth;
        rightClusterWidth += clockTextWidth + (rightClusterWidth > 0 ? clockGap : 0);
      }
      renderer.drawText(statusFontId, clockX, textY, timeBuf);
    }
  }

  // Draw Bookmark
  if (showStatusBarTextLane && isPageBookmarked) {
    const int bookmarkGap = leftClusterWidth > 0 ? bookmarkStatusIconGap : 0;
    const int bookmarkX = leftClusterX + leftClusterWidth + bookmarkGap;
    const int bookmarkY = laneTop + (laneHeight - bookmarkStatusIconHeight) / 2;
    drawBookmarkStatusIcon(renderer, bookmarkX, bookmarkY);
    leftClusterWidth += bookmarkStatusIconWidth + bookmarkGap;
  }

  // Draw Title. Hiding the battery frees the left side for book/chapter text.
  if (sb.showsTitle() && !title.empty()) {
    textY -= textYOffset;
    int titleX;
    int availableTitleSpace;
    bool centerTitle = sb.showBattery;
    if (!centerTitle) {
      constexpr int titleGap = 4;  // Compact separation from clock, bookmark and progress.
      titleX = leftClusterX + leftClusterWidth + (leftClusterWidth > 0 ? titleGap : 0);
      const int titleRight = rightClusterX - rightClusterWidth - (rightClusterWidth > 0 ? titleGap : 0);
      availableTitleSpace = titleRight - titleX;
    } else {
      // Keep the original centered layout while the battery is visible.
      const int screenWidth = renderer.getScreenWidth() - metrics.statusBarHorizontalMargin * 2 -
                              orientedMarginLeft - orientedMarginRight;
      const int titleGap = renderer.getScreenWidth() <= 320 ? 6 : 30;
      const int leftMargin = leftClusterWidth + titleGap;
      const int rightMargin = rightClusterWidth + titleGap;
      int adjustedLeft = std::max(leftMargin, rightMargin);
      availableTitleSpace = screenWidth - 2 * adjustedLeft;
      if (renderer.getTextWidth(statusFontId, title.c_str()) > availableTitleSpace) {
        availableTitleSpace = screenWidth - leftMargin - rightMargin;
        adjustedLeft = leftMargin;
      }
      titleX = adjustedLeft + metrics.statusBarHorizontalMargin + orientedMarginLeft;
    }
    // Never pass a negative width to truncation on a crowded status bar.
    if (availableTitleSpace > 0) {
      title = renderer.truncatedText(statusFontId, title.c_str(), availableTitleSpace);
      const int titleWidth = renderer.getTextWidth(statusFontId, title.c_str());
      if (centerTitle) titleX += (availableTitleSpace - titleWidth) / 2;
      renderer.drawText(statusFontId, titleX, textY, title.c_str());
    }
  }
}

void BaseTheme::drawHelpText(const GfxRenderer& renderer, Rect rect, const char* label) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  auto truncatedLabel =
      renderer.truncatedText(SMALL_FONT_ID, label, rect.width - metrics.contentSidePadding * 2, EpdFontFamily::REGULAR);
  renderer.drawCenteredText(SMALL_FONT_ID, rect.y, truncatedLabel.c_str());
}

void BaseTheme::drawTextField(const GfxRenderer& renderer, Rect rect, const int textWidth, bool cursorMode,
                              int contentStartX, int contentWidth) const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int lineHeight = renderer.getLineHeight(UI_12_FONT_ID);
  const int lineY = rect.y + rect.height + lineHeight + metrics.verticalSpacing;
  const int thickness = cursorMode ? metrics.textFieldCursorThickness : metrics.textFieldNormalThickness;
  if (contentWidth > 0) {
    renderer.drawLine(rect.x + contentStartX, lineY,
                      rect.x + contentStartX + contentWidth + metrics.textFieldLineEndOffset, lineY, thickness, true);
  } else {
    const int lineW = textWidth + metrics.textFieldHorizontalPadding * 2;
    const int lineStart = rect.x + (rect.width - lineW) / 2;
    renderer.drawLine(lineStart, lineY, lineStart + lineW + metrics.textFieldLineEndOffset, lineY, thickness, true);
  }
}
