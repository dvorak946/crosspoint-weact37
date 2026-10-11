#include "DictionaryFont.h"
#include "DictionarySettings.h"
#include "SdCardFontSystem.h"
#include "ReaderFontSizes.h"
#include "fontIds.h"
#include "compactReaderFontIds.h"
#include <EpdFontFamily.h>
#include <GfxRenderer.h>
#include <SdCardFont.h>
#include <SdCardFontRegistry.h>
#include <TtfEpdFont.h>
#include <Logging.h>
#include <algorithm>
#include <iterator>
#include <new>
#include <builtinFonts/dictionary_notosans_8_regular.h>
#include <builtinFonts/dictionary_notosans_8_bold.h>
#include <builtinFonts/dictionary_notosans_8_italic.h>
#include <builtinFonts/dictionary_notosans_8_bolditalic.h>
#include <builtinFonts/dictionary_notosans_9_regular.h>
#include <builtinFonts/dictionary_notosans_9_bold.h>
#include <builtinFonts/dictionary_notosans_9_italic.h>
#include <builtinFonts/dictionary_notosans_9_bolditalic.h>
#include <builtinFonts/compact_notoserif_8_regular.h>
#include <builtinFonts/compact_notoserif_8_bold.h>
#include <builtinFonts/compact_notoserif_8_italic.h>
#include <builtinFonts/compact_notoserif_8_bolditalic.h>
#include <builtinFonts/compact_notoserif_9_regular.h>
#include <builtinFonts/compact_notoserif_9_bold.h>
#include <builtinFonts/compact_notoserif_9_italic.h>
#include <builtinFonts/compact_notoserif_9_bolditalic.h>

namespace {
constexpr uint8_t builtinSizes[] = {8, 9, 10, 11, 12, 14, 16, 18};
constexpr int SANS_8 = 0x44530008;
constexpr int SANS_9 = 0x44530009;
constexpr int SERIF_8 = 0x44520008;
constexpr int SERIF_9 = 0x44520009;

void registerSmallFonts(GfxRenderer& renderer) {
  static EpdFont SANS_8_regular(&dictionary_notosans_8_regular);
  static EpdFont SANS_8_bold(&dictionary_notosans_8_bold);
  static EpdFont SANS_8_italic(&dictionary_notosans_8_italic);
  static EpdFont SANS_8_bolditalic(&dictionary_notosans_8_bolditalic);
  if (!renderer.getFontMap().count(SANS_8)) renderer.insertFont(SANS_8, EpdFontFamily(&SANS_8_regular, &SANS_8_bold, &SANS_8_italic, &SANS_8_bolditalic));
  static EpdFont SANS_9_regular(&dictionary_notosans_9_regular);
  static EpdFont SANS_9_bold(&dictionary_notosans_9_bold);
  static EpdFont SANS_9_italic(&dictionary_notosans_9_italic);
  static EpdFont SANS_9_bolditalic(&dictionary_notosans_9_bolditalic);
  if (!renderer.getFontMap().count(SANS_9)) renderer.insertFont(SANS_9, EpdFontFamily(&SANS_9_regular, &SANS_9_bold, &SANS_9_italic, &SANS_9_bolditalic));
  static EpdFont SERIF_8_regular(&compact_notoserif_8_regular);
  static EpdFont SERIF_8_bold(&compact_notoserif_8_bold);
  static EpdFont SERIF_8_italic(&compact_notoserif_8_italic);
  static EpdFont SERIF_8_bolditalic(&compact_notoserif_8_bolditalic);
  if (!renderer.getFontMap().count(SERIF_8)) renderer.insertFont(SERIF_8, EpdFontFamily(&SERIF_8_regular, &SERIF_8_bold, &SERIF_8_italic, &SERIF_8_bolditalic));
  static EpdFont SERIF_9_regular(&compact_notoserif_9_regular);
  static EpdFont SERIF_9_bold(&compact_notoserif_9_bold);
  static EpdFont SERIF_9_italic(&compact_notoserif_9_italic);
  static EpdFont SERIF_9_bolditalic(&compact_notoserif_9_bolditalic);
  if (!renderer.getFontMap().count(SERIF_9)) renderer.insertFont(SERIF_9, EpdFontFamily(&SERIF_9_regular, &SERIF_9_bold, &SERIF_9_italic, &SERIF_9_bolditalic));
}

// A separate registration, even if the reader happens to use the same source.
// It must not replace or free the font owned by the activity beneath us.
int unusedId(const GfxRenderer& renderer) {
  int id = 0x44494354;
  while (renderer.getFontMap().count(id)) ++id;
  return id;
}
}  // namespace

DictionaryFont::DictionaryFont() = default;
DictionaryFont::~DictionaryFont() = default;

int DictionaryFont::builtinId(uint8_t family, uint8_t size) {
  const uint8_t pt = snapToNearestPointSize(builtinSizes, std::size(builtinSizes), size);
  const bool serif = family == DictionarySettings::Serif;
  switch (pt) {
    case 8: return serif ? SERIF_8 : SANS_8;
    case 9: return serif ? SERIF_9 : SANS_9;
    case 10: return serif ? COMPACT_NOTOSERIF_10_FONT_ID : COMPACT_NOTOSANS_10_FONT_ID;
    case 11: return serif ? COMPACT_NOTOSERIF_11_FONT_ID : COMPACT_NOTOSANS_11_FONT_ID;
    case 12: return serif ? NOTOSERIF_12_FONT_ID : NOTOSANS_12_FONT_ID;
    case 14: return serif ? NOTOSERIF_14_FONT_ID : NOTOSANS_14_FONT_ID;
    case 16: return serif ? NOTOSERIF_16_FONT_ID : NOTOSANS_16_FONT_ID;
    default: return serif ? NOTOSERIF_18_FONT_ID : NOTOSANS_18_FONT_ID;
  }
}

int DictionaryFont::load(GfxRenderer& renderer, const DictionarySettings& settings) {
  unload(renderer);
  registerSmallFonts(renderer);
  usedFallback_ = false;
  actualPointSize_ = snapToNearestPointSize(builtinSizes, std::size(builtinSizes), settings.pointSize);
  const int fallback = builtinId(settings.family, actualPointSize_);
  if (!settings.sdFamily[0]) return fallback;

  sdFontSystem.refreshIfDirty();
  const auto* family = sdFontSystem.registry().findFamily(settings.sdFamily);
  if (family) {
#if CROSSPOINT_VECTOR_FONTS
    if (family->vector) {
      ttfFont_.reset(new (std::nothrow) TtfEpdFont());
      if (ttfFont_) {
        for (const auto& file : family->files) {
          const uint8_t role = file.style < 4 ? file.style : 0;
          if (sourceFiles_[role]) continue;
          sourceFiles_[role] = Storage.open(file.path.c_str(), O_READ);
          if (sourceFiles_[role]) {
            ttfFont_->addStreamSource(role, &SdCardFontRegistry::halFileRead, &sourceFiles_[role],
                                     sourceFiles_[role].size());
          }
        }
        actualPointSize_ = std::max<uint8_t>(8, std::min<uint8_t>(22, settings.pointSize));
        if (sourceFiles_[0] && ttfFont_->load(actualPointSize_, true, 16 * 1024, 512)) {
          registeredId_ = unusedId(renderer);
          renderer.insertFont(registeredId_, ttfFont_->family());
          renderer.registerTtfFont(registeredId_, ttfFont_.get());
          return registeredId_;
        }
      }
    } else
#endif
    if (!family->vector) {
      const auto* file = family->findNearestSize(settings.pointSize);
      if (file) {
        sdFont_.reset(new (std::nothrow) SdCardFont());
        if (sdFont_ && sdFont_->load(file->path.c_str())) {
          actualPointSize_ = file->pointSize;
          registeredId_ = unusedId(renderer);
          renderer.registerSdCardFont(registeredId_, sdFont_.get());
          renderer.insertFont(registeredId_, EpdFontFamily(sdFont_->getEpdFont(0), sdFont_->getEpdFont(1),
                                                          sdFont_->getEpdFont(2), sdFont_->getEpdFont(3)));
          return registeredId_;
        }
      }
    }
  }
  LOG_ERR("DICTFONT", "Cannot load %s; using built-in font for this lookup", settings.sdFamily);
  unload(renderer);
  actualPointSize_ = snapToNearestPointSize(builtinSizes, std::size(builtinSizes), settings.pointSize);
  usedFallback_ = true;
  return fallback;
}

void DictionaryFont::unload(GfxRenderer& renderer) {
  if (registeredId_) {
#if CROSSPOINT_VECTOR_FONTS
    if (ttfFont_) renderer.unregisterTtfFont(registeredId_);
#endif
    renderer.removeFont(registeredId_);
    registeredId_ = 0;
  }
  sdFont_.reset();
#if CROSSPOINT_VECTOR_FONTS
  ttfFont_.reset();  // FreeType must be destroyed BEFORE its borrowed files close.
  for (auto& file : sourceFiles_) if (file) file.close();
#endif
}
