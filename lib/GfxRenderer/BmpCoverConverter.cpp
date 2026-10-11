#include "BmpCoverConverter.h"

#include <Bitmap.h>
#include <Memory.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <algorithm>
#include <cstring>

#include "CoverSizing.h"

namespace {
bool convert(HalFile& source, Print& output, int requestedWidth, int requestedHeight) {
  Bitmap bitmap(source);
  if (bitmap.parseHeaders() != BmpReaderError::Ok) return false;
  const int sourceWidth = bitmap.getWidth(), sourceHeight = bitmap.getHeight();
  int width = 0, height = 0;
  int cropX = 0, cropY = 0, cropWidth = sourceWidth, cropHeight = sourceHeight;
  if (requestedHeight > 0) {
    if (requestedWidth <= 0 || requestedWidth > 2048 || requestedHeight > 3072) return false;
    width = requestedWidth;
    height = requestedHeight;
    if (int64_t(sourceWidth) * height > int64_t(sourceHeight) * width) {
      cropWidth = std::max(1, int(int64_t(sourceHeight) * width / height));
      cropX = (sourceWidth - cropWidth) / 2;
    } else {
      cropHeight = std::max(1, int(int64_t(sourceWidth) * height / width));
      cropY = (sourceHeight - cropHeight) / 2;
    }
    // Rows are streamed in file order; preserve the same logical crop when
    // the source is bottom-up, including an odd number of removed rows.
    if (!bitmap.isTopDown()) cropY = sourceHeight - cropY - cropHeight;
  } else if (!CoverSizing::byWidth(sourceWidth, sourceHeight, requestedWidth, width, height)) return false;
  const int stride = (width + 31) / 32 * 4;
  auto packed = makeUniqueNoThrow<uint8_t[]>((sourceWidth + 3) / 4);
  auto raw = makeUniqueNoThrow<uint8_t[]>(bitmap.getRowBytes());
  auto row = makeUniqueNoThrow<uint8_t[]>(stride);
  auto sums = makeUniqueNoThrow<uint32_t[]>(width);
  Atkinson1BitDitherer dither(width);
  if (!packed || !raw || !row || !sums || !dither.isValid()) return false;
  BmpHeader header;
  createBmpHeader(&header, width, height, bitmap.isTopDown() ? BmpRowOrder::TopDown : BmpRowOrder::BottomUp);
  if (output.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header)) != sizeof(header)) return false;
  int sourceRow = -1;
  for (int y = 0; y < height; ++y) {
    const int firstY = cropY + int64_t(y) * cropHeight / height;
    const int lastY = std::min(cropY + cropHeight, std::max(firstY + 1, cropY + int(int64_t(y + 1) * cropHeight / height)));
    std::memset(sums.get(), 0, static_cast<size_t>(width) * sizeof(uint32_t));
    for (int sy = firstY; sy < lastY; ++sy) {
      while (sourceRow < sy) {
        if (bitmap.readNextRow(packed.get(), raw.get()) != BmpReaderError::Ok) return false;
        ++sourceRow;
      }
      for (int x = 0; x < width; ++x) {
        const int firstX = cropX + int64_t(x) * cropWidth / width;
        const int lastX = std::min(cropX + cropWidth, std::max(firstX + 1, cropX + int(int64_t(x + 1) * cropWidth / width)));
        for (int sx = firstX; sx < lastX; ++sx) {
          sums[x] += ((packed[sx / 4] >> (6 - 2 * (sx % 4))) & 3u) * 85u;
        }
      }
    }
    std::memset(row.get(), 0xFF, stride);
    for (int x = 0; x < width; ++x) {
      const int firstX = cropX + int64_t(x) * cropWidth / width;
      const int lastX = std::min(cropX + cropWidth, std::max(firstX + 1, cropX + int(int64_t(x + 1) * cropWidth / width)));
      const int count = (lastX - firstX) * (lastY - firstY);
      if (!dither.processPixel(static_cast<int>(sums[x] / count), x)) row[x / 8] &= ~(1u << (7 - x % 8));
    }
    dither.nextRow();
    if (output.write(row.get(), stride) != static_cast<size_t>(stride)) return false;
    if ((y & 15) == 15) vTaskDelay(1);
  }
  return true;
}
}  // namespace

bool bmpCoverTo1BitWithWidth(HalFile& source, Print& output, int width) {
  return convert(source, output, width, 0);
}

bool bmpCoverTo1BitFill(HalFile& source, Print& output, int width, int height) {
  return height > 0 && convert(source, output, width, height);
}
