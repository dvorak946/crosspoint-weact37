#pragma once

#include <string_view>

namespace FsHelpers {
// Internal storage is hidden from user-facing listings even with Show Hidden
// Files enabled. This is a listing policy, not an authentication boundary.
inline bool isInternalCachePath(std::string_view path) {
  while (!path.empty() && path.front() == '/') path.remove_prefix(1);
  constexpr std::string_view cache = ".crosspoint";
  if (path.size() < cache.size()) return false;
  for (size_t i = 0; i < cache.size(); ++i) {
    char c = path[i];
    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (c != cache[i]) return false;
  }
  return path.size() == cache.size() || path[cache.size()] == '/';
}

inline bool isInternalCacheEntry(std::string_view directory, std::string_view name) {
  if (isInternalCachePath(directory)) return true;
  while (!directory.empty() && directory.front() == '/') directory.remove_prefix(1);
  return directory.empty() && isInternalCachePath(name);
}
}  // namespace FsHelpers
