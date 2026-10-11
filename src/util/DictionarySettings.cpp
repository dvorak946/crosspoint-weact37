#include "DictionarySettings.h"

#include <I18n.h>
#include <algorithm>
#include <cstring>

void DictionarySettings::ensureLoaded() {
  if (loaded_) return;
  loadFromFile();  // Missing file on first use leaves compact defaults intact.
  loaded_ = true;
}

void DictionarySettings::toJson(JsonDocument& doc) const {
  doc["family"] = family;
  doc["pointSize"] = pointSize;
  doc["sdFamily"] = sdFamily;
  doc["margin"] = margin;
  doc["lineSpacing"] = lineSpacing;
  doc["paragraphSpacing"] = paragraphSpacing;
  doc["alignment"] = alignment;
}

bool DictionarySettings::fromJson(JsonVariantConst doc) {
  if (!doc.is<JsonObjectConst>()) return false;
  const auto value = [&](const char* key, int min, int max, int fallback) -> uint8_t {
    const int v = doc[key] | fallback;
    return static_cast<uint8_t>(v >= min && v <= max ? v : fallback);
  };
  family = value("family", Sans, Serif, Sans);
  pointSize = value("pointSize", 6, 48, 8);
  margin = value("margin", 5, 20, 10);
  lineSpacing = value("lineSpacing", Tight, Wide, Tight);
  paragraphSpacing = value("paragraphSpacing", 0, 1, 0);
  alignment = value("alignment", Left, Justify, Left);
  const char* name = doc["sdFamily"] | "";
  std::strncpy(sdFamily, name, sizeof(sdFamily) - 1);
  sdFamily[sizeof(sdFamily) - 1] = '\0';
  return true;
}

float DictionarySettings::lineCompression() const {
  switch (lineSpacing) {
    case Normal: return 1.1f;
    case Wide: return 1.3f;
    default: return 0.95f;
  }
}

const char* DictionarySettings::fontFamilyName() const {
  return sdFamily[0] ? sdFamily : family == Serif ? "Noto Serif" : "Noto Sans";
}

const char* DictionarySettings::title() {
  return I18N.getLanguage() == Language::ID ? "Pengaturan Kamus" : "Dictionary Settings";
}
