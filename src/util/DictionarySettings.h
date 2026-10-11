#pragma once

#include <PersistableStore.h>
#include <cstdint>

// Kept outside book settings: choosing a dictionary font must never change
// the reader font, its loaded SD family, or a book's pagination/cache key.
class DictionarySettings final : public PersistableStore<DictionarySettings> {
  friend class PersistableStore<DictionarySettings>;
  DictionarySettings() = default;
  bool loaded_ = false;

 public:
  enum Family : uint8_t { Sans, Serif };
  enum Spacing : uint8_t { Tight, Normal, Wide };
  enum Alignment : uint8_t { Left, Justify };

  uint8_t family = Sans;
  uint8_t pointSize = 8;
  char sdFamily[32] = "";
  uint8_t margin = 10;
  uint8_t lineSpacing = Tight;
  uint8_t paragraphSpacing = 0;  // 0=compact, 1=normal
  uint8_t alignment = Left;

  static const char* getFilePath() { return "/.crosspoint/dictionary-settings.json"; }
  void ensureLoaded();
  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);
  float lineCompression() const;
  const char* fontFamilyName() const;
  static const char* title();
};

#define DICTIONARY_SETTINGS DictionarySettings::getInstance()
