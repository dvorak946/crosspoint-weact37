#pragma once

#include <HalStorage.h>
#include <VectorFontSupport.h>
#include <cstdint>
#include <memory>

class GfxRenderer;
class SdCardFont;
class TtfEpdFont;
class DictionarySettings;

// Activity-owned font registration. Removes only this dictionary's font,
// leaving the reader's SD/vector font and UI fallbacks alive beneath it.
class DictionaryFont {
 public:
  DictionaryFont();
  ~DictionaryFont();
  DictionaryFont(const DictionaryFont&) = delete;
  DictionaryFont& operator=(const DictionaryFont&) = delete;
  int load(GfxRenderer& renderer, const DictionarySettings& settings);
  void unload(GfxRenderer& renderer);
  uint8_t actualPointSize() const { return actualPointSize_; }
  bool usedFallback() const { return usedFallback_; }
  static int builtinId(uint8_t family, uint8_t size);

 private:
  int registeredId_ = 0;
  uint8_t actualPointSize_ = 8;
  bool usedFallback_ = false;
  std::unique_ptr<SdCardFont> sdFont_;
#if CROSSPOINT_VECTOR_FONTS
  std::unique_ptr<TtfEpdFont> ttfFont_;
  HalFile sourceFiles_[4];
#endif
};
