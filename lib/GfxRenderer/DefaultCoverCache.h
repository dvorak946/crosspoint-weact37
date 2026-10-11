#pragma once
#include <string>

// Empty means unavailable: caller uses the built-in placeholder. The source
// is never modified. One cached image is reused across books without covers.
std::string prepareDefaultHomeCover(int width, int height);
