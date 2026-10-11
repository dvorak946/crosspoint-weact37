#pragma once

#include "components/themes/BaseTheme.h"

class GfxRenderer;

namespace RoundedRaffMetrics {
constexpr ThemeMetrics values = {.batteryWidth = 12,
                                 .batteryHeight = 8,
                                 // Compact portrait layout for the WeAct 3.7-inch panel (240 x 416).
                                 .topPadding = 0,
                                 .batteryBarHeight = 15,
                                 .headerHeight = 64,
                                 .verticalSpacing = 6,
                                 .previewPadding = 8,
                                 .previewHeightPercent = 30,
                                 .contentSidePadding = 10,
                                 .listRowHeight = 28,
                                 .listWithSubtitleRowHeight = 44,
                                 .listRowGap = 4,
                                 .listRowRadius = 12,
                                 .listInset = 10,
                                 .listSidePadding = 10,
                                 .listSelectionStyle = 0,  // invert fill (black card)
                                 .listScrollWidth = 4,
                                 .listScrollSide = 0,
                                 .listTitleBold = true,
                                 .headerSidePadding = 10,
                                 .headerUnderlineSize = 0,
                                 .headerTitleAlign = 0,  // left
                                 .headerBatterySide = 0,
                                 // A centered clock needs no left title reserve, so RoundedRaff
                                 // can show one after all.
                                 .headerShowsClock = true,
                                 .menuRowHeight = 25,  // one horizontal Home tab band
                                 .menuSpacing = 0,
                                 .tabSpacing = 6,
                                 .tabBarHeight = 32,
                                 .coverGridTabBarHeight = 32,
                                 .tabPillFullSlot = true,
                                 .scrollBarWidth = 4,
                                 .scrollBarRightOffset = 5,
                                 // Keep covers below the status strip; the actual cover/title height is
                                 // shared by the fixed square art and the metadata reservation.
                                 .homeTopPadding = 18,
                                 .homeCoverHeight = 220,
                                 .homeCoverTileHeight = 306,  // 220px cover + 2px gap + 84px metadata
                                 .homeRecentBooksCount = 1,
                                 .homeContinueReadingInMenu = false,
                                 .homeMenuTopOffset = 2,
                                 .buttonHintsHeight = 32,
                                 .sideButtonHintsWidth = 20,
                                 .progressBarHeight = 8,
                                 .progressBarMarginTop = 1,
                                 .statusBarHorizontalMargin = 5,
                                 .statusBarVerticalMargin = 19,
                                 .keyboardKeyHeight = 32,
                                 .keyboardKeySpacing = 0,
                                 .keyboardCenteredText = false,
                                 .keyboardVerticalOffset = -6,
                                 .keyboardTextFieldWidthPercent = 85,
                                 .keyboardWidthPercent = 94,
                                 .popupTopOffsetRatio = 0.075f,
                                 .popupMarginX = 10,
                                 .popupMarginY = 10,
                                 .popupFrameThickness = 2,
                                 .popupCornerRadius = 12,
                                 .popupTextBold = true,
                                 .popupTextInverted = false,
                                 .popupTextBaselineOffsetY = -2,
                                 .popupProgressBarHeight = 4,
                                 .popupProgressDrawOutline = true,
                                 .popupProgressClampPercent = true,
                                 .popupProgressFillInverted = false,
                                 .popupProgressOutlineInverted = false,
                                 .optionPopupItemSpacing = 6,
                                 .optionPopupInnerPadding = 8,
                                 .optionPopupSelectionVPadding = 4,
                                 .optionPopupDialogSideMargin = 10,
                                 .textFieldHorizontalPadding = 8,
                                 .textFieldNormalThickness = 2,
                                 .textFieldCursorThickness = 3,
                                 .textFieldLineEndOffset = -1,
                                 .controlRadius = 12,
                                 .sheetRadius = 12,
                                 .capsuleRadius = 255};
}

class RoundedRaffTheme : public BaseTheme {
 public:
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle = nullptr,
                  bool backButton = true) const override;
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;
  int getMenuRowHeight(const GfxRenderer& renderer) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
  void drawTextField(const GfxRenderer& renderer, Rect rect, int textWidth, bool cursorMode = false,
                     int contentStartX = 0, int contentWidth = 0) const override;
  void drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                       const char* btn4) const override;
  bool homeMenuShowsContinueReading() const { return false; }


 private:
  // Home calls cover then menu in the same render. Defer the cover until the
  // menu band is known. Its height stays fixed with or without plugins.
  mutable std::function<void(int)> pendingHomeCover_;
  mutable int cachedCoverWidth_ = 0;
  mutable int cachedCoverHeight_ = 0;
  mutable int homeBookProgress_ = -1;
  mutable std::string progressBookPath_;
  void drawHomeCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                     int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool bufferRestored,
                     const std::function<bool()>& storeCoverBuffer) const;
};
