#include "QrUtils.h"

#include <Utf8.h>
#include <qrcode.h>

#include <algorithm>
#include <memory>

#include "Logging.h"

void QrUtils::drawQrCode(const GfxRenderer& renderer, const Rect& bounds, const std::string& textPayload) {
  // Dynamically calculate the QR code version based on text length
  // Version 4 holds ~114 bytes, Version 10 ~395, Version 20 ~1066, up to 40
  // qrcode.h max version is 40.
  // Formula: approx version = size / 26 + 1 (very rough estimate, better to find best fit)
  size_t len = textPayload.length();

  // Truncate to max QR capacity at a UTF-8 safe boundary to avoid splitting multi-byte sequences
  static constexpr size_t MAX_QR_CAPACITY = 2953;  // Version 40, ECC_LOW, byte mode
  std::string truncated;
  const char* payload = textPayload.c_str();
  if (len > MAX_QR_CAPACITY) {
    len = utf8SafeTruncateBuffer(textPayload.c_str(), static_cast<int>(MAX_QR_CAPACITY));
    truncated = textPayload.substr(0, len);
    payload = truncated.c_str();
  }

  int version = 4;
  if (len > 114) version = 10;
  if (len > 395) version = 20;
  if (len > 1066) version = 30;
  if (len > 2110) version = 40;

  // Make sure we have a large enough buffer on the heap to avoid blowing the stack
  uint32_t bufferSize = qrcode_getBufferSize(version);
  auto qrcodeBytes = std::make_unique<uint8_t[]>(bufferSize);

  QRCode qrcode;
  // Initialize the QR code. We use ECC_LOW for max capacity.
  int8_t res = qrcode_initText(&qrcode, qrcodeBytes.get(), version, ECC_LOW, payload);

  if (res == 0) {
    // Clip the requested box to the physical display before choosing a scale.
    const int left = std::max(0, bounds.x);
    const int top = std::max(0, bounds.y);
    const int right = std::min(renderer.getScreenWidth(), bounds.x + bounds.width);
    const int bottom = std::min(renderer.getScreenHeight(), bounds.y + bounds.height);
    const int width = right - left;
    const int height = bottom - top;
    if (width <= 0 || height <= 0) {
      LOG_ERR("QR", "QR bounds are outside the screen");
      return;
    }

    // QR readers need four blank modules around every edge. Include this
    // quiet zone when fitting the QR, with an integer number of pixels/module.
    constexpr int QUIET_ZONE = 4;
    const int totalModules = qrcode.size + 2 * QUIET_ZONE;
    const int px = std::min(width, height) / totalModules;
    if (px < 1) {
      LOG_ERR("QR", "QR bounds too small: %d x %d for %d modules", width, height, totalModules);
      return;
    }

    renderer.fillRect(left, top, width, height, false);
    const int qrDisplaySize = totalModules * px;
    const int xOff = left + (width - qrDisplaySize) / 2 + QUIET_ZONE * px;
    const int yOff = top + (height - qrDisplaySize) / 2 + QUIET_ZONE * px;

    // Draw the QR Code
    for (uint8_t cy = 0; cy < qrcode.size; cy++) {
      for (uint8_t cx = 0; cx < qrcode.size; cx++) {
        if (qrcode_getModule(&qrcode, cx, cy)) {
          renderer.fillRect(xOff + px * cx, yOff + px * cy, px, px, true);
        }
      }
    }
  } else {
    // If it fails (e.g. text too large), log an error
    LOG_ERR("QR", "Text too large for QR Code version %d", version);
  }
}
