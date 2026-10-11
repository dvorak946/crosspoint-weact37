#include "DefaultCoverCache.h"
#include "BmpCoverConverter.h"
#include "CoverCache.h"

#include <HalStorage.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
constexpr const char* sourcePath = "/.crosspoint/default_cover.bmp";
constexpr const char* cachePath = "/.crosspoint/home_default_cover_1bit_v5.bmp";
constexpr const char* keyPath = "/.crosspoint/home_default_cover_v5.key";
constexpr const char* tempPath = "/.crosspoint/home_default_cover_1bit_v5.bmp.tmp";
constexpr const char* tempKeyPath = "/.crosspoint/home_default_cover_v5.key.tmp";
}

std::string prepareDefaultHomeCover(int width, int height) {
  if (width <= 0 || width > 2048 || height <= 0 || height > 3072) return {};
  HalFile source;
  if (!Storage.openFileForRead("COVER", sourcePath, source)) return {};
  // A streaming fingerprint notices replacement of default_cover.bmp even
  // if its filename and byte count stay the same. No source-sized RAM buffer.
  uint32_t hash = 2166136261u;
  std::array<uint8_t, 512> chunk{};
  size_t remaining = source.size();
  unsigned reads = 0;
  while (remaining) {
    const size_t length = std::min(remaining, chunk.size());
    if (source.read(chunk.data(), length) != static_cast<int>(length)) return {};
    for (size_t i = 0; i < length; ++i) hash = (hash ^ chunk[i]) * 16777619u;
    remaining -= length;
    if ((++reads & 15) == 0) vTaskDelay(1);
  }
  const std::array<uint32_t, 3> key{hash, static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  HalFile previous;
  std::array<uint32_t, 3> saved{};
  if (Storage.openFileForRead("COVER", keyPath, previous) && previous.size() == sizeof(saved) &&
      previous.read(reinterpret_cast<uint8_t*>(saved.data()), sizeof(saved)) == static_cast<int>(sizeof(saved)) && saved == key &&
      CoverCache::valid(cachePath, width, height)) return cachePath;
  previous.close();
  if (!source.seek(0)) return {};
  HalFile output;
  if (!Storage.openFileForWrite("COVER", tempPath, output)) return {};
  const bool converted = bmpCoverTo1BitFill(source, output, width, height);
  output.close();
  source.close();
  if (!converted || !CoverCache::commit(tempPath, cachePath, width, height)) {
    Storage.remove(tempPath);
    return {};
  }
  HalFile stamp;
  bool keyOk = Storage.openFileForWrite("COVER", tempKeyPath, stamp);
  if (keyOk) keyOk = stamp.write(reinterpret_cast<const uint8_t*>(key.data()), sizeof(key)) == sizeof(key);
  stamp.close();
  if (keyOk) keyOk = Storage.replaceFile(tempKeyPath, keyPath);
  if (!keyOk) {
    Storage.remove(tempKeyPath);
    Storage.remove(keyPath);  // Prevent stale metadata from validating new pixels.
  }
  return cachePath;
}
