#include "Lyra3CoversTheme.h"

#include <GfxRenderer.h>
#include <HalStorage.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/icons/blocks.h"
#include "components/icons/book.h"
#include "components/icons/bookmark.h"
#include "components/icons/cover.h"
#include "components/icons/folder.h"
#include "components/icons/hotspot.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/transfer.h"
#include "components/icons/wifi.h"
#include "fontIds.h"
#include "customUiFontIds.h"

namespace {
constexpr int hPaddingInSelection = 8;
constexpr int cornerRadius = 6;
constexpr int mainMenuIconSize = 32;
constexpr int titleGap = 4;
constexpr int titleBottomPadding = 4;
const uint8_t* iconForName(UIIcon icon) {
  switch (icon) {
    case UIIcon::Folder:
      return FolderIcon;
    case UIIcon::Book:
      return BookIcon;
    case UIIcon::Recent:
      return RecentIcon;
    case UIIcon::Settings:
      return Settings2Icon;
    case UIIcon::Transfer:
      return TransferIcon;
    case UIIcon::Library:
      return LibraryIcon;
    case UIIcon::Plugins:
      return BlocksIcon;
    case UIIcon::Wifi:
      return WifiIcon;
    case UIIcon::Hotspot:
      return HotspotIcon;
    case UIIcon::Bookmark:
      return BookmarkIcon;
    case UIIcon::Blocks:
      return BlocksIcon;
    default:
      return nullptr;
  }
}
}  // namespace

int Lyra3CoversTheme::homeCoverThumbHeight(const GfxRenderer& renderer) const {
  const int tileWidth = (renderer.getScreenWidth() - 2 * Lyra3CoversMetrics::values.contentSidePadding) /
                        Lyra3CoversMetrics::values.homeRecentBooksCount;
  // Keep the existing 1:1 dither rendering; crop taller thumbnails into the slot.
  return std::max(Lyra3CoversMetrics::values.homeCoverHeight,
                  std::max(1, tileWidth - 2 * hPaddingInSelection) * 5 / 3 + 2);
}

void Lyra3CoversTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect,
                                         const std::vector<RecentBook>& recentBooks, const int selectorIndex,
                                         bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                                         std::function<bool()> storeCoverBuffer) const {
  const auto& metrics = Lyra3CoversMetrics::values;
  const int tileWidth = (rect.width - 2 * metrics.contentSidePadding) / metrics.homeRecentBooksCount;
  const int coverSlotWidth = tileWidth - 2 * hPaddingInSelection;
  const int coverSlotHeight = std::min(metrics.homeCoverHeight, rect.height - 2 * hPaddingInSelection);
  if (coverSlotWidth <= 0 || coverSlotHeight <= 0) return;

  if (recentBooks.empty()) {
    drawEmptyRecents(renderer, rect);
    return;
  }

  const int bookCount = std::min(static_cast<int>(recentBooks.size()), metrics.homeRecentBooksCount);
  if (!coverRendered) {
    for (int i = 0; i < bookCount; ++i) {
      const int tileX = rect.x + metrics.contentSidePadding + tileWidth * i;
      const Rect slot{tileX + hPaddingInSelection, rect.y + hPaddingInSelection,
                      coverSlotWidth, coverSlotHeight};
      bool hasCover = false;
      if (!recentBooks[i].coverBmpPath.empty()) {
        const std::string path = UITheme::getCoverThumbPath(recentBooks[i].coverBmpPath,
                                                          homeCoverThumbHeight(renderer));
        HalFile file;
        if (Storage.openFileForRead("HOME", path, file)) {
          Bitmap bitmap(file);
          if (bitmap.parseHeaders() == BmpReaderError::Ok) {
            hasCover = drawCoverThumbFill(renderer, bitmap, slot);
          }
          file.close();
        }
      }

      if (!hasCover) {
        // Center the 32px fallback icon inside the narrow 57px cover slot.
        const int fillTop = slot.height / 3;
        renderer.fillRect(slot.x, slot.y + fillTop, slot.width, slot.height - fillTop, true);
        if (slot.width >= mainMenuIconSize && slot.height >= mainMenuIconSize) {
          renderer.drawIcon(CoverIcon, slot.x + (slot.width - mainMenuIconSize) / 2,
                            slot.y + (slot.height - mainMenuIconSize) / 2, mainMenuIconSize);
        }
      }
      renderer.drawRect(slot.x, slot.y, slot.width, slot.height, true);
    }
    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  }

  const int titleFont = UI_6_FONT_ID;
  const int titleLineHeight = std::max(1, renderer.getLineHeight(titleFont));
  const int titleTop = rect.y + hPaddingInSelection + coverSlotHeight + titleGap;
  const int titleSpace = rect.y + rect.height - titleBottomPadding - titleTop;
  const int maxLines = std::min(3, std::max(0, titleSpace / titleLineHeight));

  for (int i = 0; i < bookCount; ++i) {
    const int tileX = rect.x + metrics.contentSidePadding + tileWidth * i;
    const int titleBoxTop = rect.y + hPaddingInSelection + coverSlotHeight;
    const int titleBoxHeight = rect.y + rect.height - titleBoxTop;
    if (selectorIndex == i) {
      // Fill only the border and title area, preserving the cached cover pixels.
      renderer.fillRoundedRect(tileX, rect.y, tileWidth, hPaddingInSelection, cornerRadius,
                               true, true, false, false, Color::LightGray);
      renderer.fillRectDither(tileX, rect.y + hPaddingInSelection, hPaddingInSelection,
                              coverSlotHeight, Color::LightGray);
      renderer.fillRectDither(tileX + tileWidth - hPaddingInSelection, rect.y + hPaddingInSelection,
                              hPaddingInSelection, coverSlotHeight, Color::LightGray);
      if (titleBoxHeight > 0) {
        renderer.fillRoundedRect(tileX, titleBoxTop, tileWidth, titleBoxHeight, cornerRadius,
                                 false, false, true, true, Color::LightGray);
      }
    }
    if (maxLines == 0) continue;
    const auto lines = renderer.wrappedText(titleFont, recentBooks[i].title.c_str(), coverSlotWidth, maxLines);
    int titleY = titleTop;
    for (const auto& line : lines) {
      renderer.drawText(titleFont, tileX + hPaddingInSelection, titleY, line.c_str(), true);
      titleY += titleLineHeight;
    }
  }
}

void Lyra3CoversTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                     const std::function<std::string(int index)>& buttonLabel,
                                     const std::function<UIIcon(int index)>& rowIcon) const {
  const auto& metrics = Lyra3CoversMetrics::values;
  const int tileWidth = rect.width - 2 * metrics.contentSidePadding;
  if (tileWidth <= 0) return;
  const int rowHeight = getMenuRowHeight(renderer);
  const int fontId = UI_10_FONT_ID;
  const int lineHeight = renderer.getLineHeight(fontId);
  const int bottom = std::min(rect.y + rect.height, renderer.getScreenHeight() - metrics.buttonHintsHeight);

  for (int i = 0; i < buttonCount; ++i) {
    const Rect tile{rect.x + metrics.contentSidePadding, rect.y + i * (rowHeight + metrics.menuSpacing),
                    tileWidth, rowHeight};
    if (tile.y + tile.height > bottom) break;
    if (selectedIndex == i) {
      renderer.fillRoundedRect(tile.x, tile.y, tile.width, tile.height, cornerRadius, Color::LightGray);
    }
    int textX = tile.x + hPaddingInSelection;
    if (rowIcon) {
      const uint8_t* icon = iconForName(rowIcon(i));
      if (icon && tile.height >= mainMenuIconSize) {
        renderer.drawIcon(icon, textX, tile.y + (tile.height - mainMenuIconSize) / 2, mainMenuIconSize);
        textX += mainMenuIconSize + hPaddingInSelection + 2;
      }
    }
    const int textWidth = tile.x + tile.width - hPaddingInSelection - textX;
    if (textWidth <= 0) continue;
    const auto fitted = renderer.truncatedText(fontId, buttonLabel(i).c_str(), textWidth);
    renderer.drawText(fontId, textX, tile.y + (tile.height - lineHeight) / 2, fitted.c_str(), true);
  }
}
