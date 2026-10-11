#pragma once

#include <EpdFontFamily.h>
#include <GfxRenderer.h>

#include "customUiFontIds.h"
#include <builtinFonts/ubuntu_6_regular.h>
#include <builtinFonts/ubuntu_6_bold.h>
#include <builtinFonts/ubuntu_7_regular.h>
#include <builtinFonts/ubuntu_7_bold.h>
#include <builtinFonts/ubuntu_8_regular.h>
#include <builtinFonts/ubuntu_8_bold.h>
#include <builtinFonts/ubuntu_9_regular.h>
#include <builtinFonts/ubuntu_9_bold.h>

// Register once during setup, beside the existing built-in font registrations.
// Function-local statics keep the font objects alive for the renderer's lifetime.
inline void registerCustomUiFonts(GfxRenderer& renderer) {
  static EpdFont ui6Regular(&ubuntu_6_regular);
  static EpdFont ui6Bold(&ubuntu_6_bold);
  static EpdFontFamily ui6Family(&ui6Regular, &ui6Bold);
  renderer.insertFont(UI_6_FONT_ID, ui6Family);
  static EpdFont ui7Regular(&ubuntu_7_regular);
  static EpdFont ui7Bold(&ubuntu_7_bold);
  static EpdFontFamily ui7Family(&ui7Regular, &ui7Bold);
  renderer.insertFont(UI_7_FONT_ID, ui7Family);
  static EpdFont ui8Regular(&ubuntu_8_regular);
  static EpdFont ui8Bold(&ubuntu_8_bold);
  static EpdFontFamily ui8Family(&ui8Regular, &ui8Bold);
  renderer.insertFont(UI_8_FONT_ID, ui8Family);
  static EpdFont ui9Regular(&ubuntu_9_regular);
  static EpdFont ui9Bold(&ubuntu_9_bold);
  static EpdFontFamily ui9Family(&ui9Regular, &ui9Bold);
  renderer.insertFont(UI_9_FONT_ID, ui9Family);
}
