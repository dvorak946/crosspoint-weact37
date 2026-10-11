#pragma once

#include <Bitmap.h>
#include <HalStorage.h>

#include <string>

namespace CoverCache {
inline bool valid(const std::string& path, int width, int height = 0) {
  if (width <= 0) return false;
  HalFile file;
  if (!Storage.openFileForRead("COVER", path, file)) return false;
  Bitmap bitmap(file);
  const bool headersOk = bitmap.parseHeaders() == BmpReaderError::Ok && bitmap.is1Bit() &&
                         bitmap.getWidth() == width && bitmap.getHeight() > 0 &&
                         (height <= 0 || bitmap.getHeight() == height);
  const bool complete = headersOk && file.size() == 62u + static_cast<size_t>(bitmap.getRowBytes()) * bitmap.getHeight();
  file.close();
  return complete;
}

inline bool commit(const std::string& temporary, const std::string& path, int width, int height = 0) {
  if (valid(temporary, width, height) && Storage.replaceFile(temporary.c_str(), path.c_str())) return true;
  Storage.remove(temporary.c_str());
  return false;
}
}  // namespace CoverCache
