#pragma once

class HalFile;
class Print;

// Resize an existing BMP using a few row buffers, then dither to 1-bit.
bool bmpCoverTo1BitWithWidth(HalFile& source, Print& output, int width);

// Crop the undithered source to the destination aspect, then resize and
// dither. Used for the user-supplied default image, which fills the slot.
bool bmpCoverTo1BitFill(HalFile& source, Print& output, int width, int height);
