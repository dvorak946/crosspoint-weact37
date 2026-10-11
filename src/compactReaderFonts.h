#pragma once

#include <EpdFontFamily.h>
#include <GfxRenderer.h>

#include "compactReaderFontIds.h"
#include <builtinFonts/compact_notoserif_10_regular.h>
#include <builtinFonts/compact_notoserif_10_bold.h>
#include <builtinFonts/compact_notoserif_10_italic.h>
#include <builtinFonts/compact_notoserif_10_bolditalic.h>
#include <builtinFonts/compact_notoserif_11_regular.h>
#include <builtinFonts/compact_notoserif_11_bold.h>
#include <builtinFonts/compact_notoserif_11_italic.h>
#include <builtinFonts/compact_notoserif_11_bolditalic.h>
#include <builtinFonts/compact_notosans_10_regular.h>
#include <builtinFonts/compact_notosans_10_bold.h>
#include <builtinFonts/compact_notosans_10_italic.h>
#include <builtinFonts/compact_notosans_10_bolditalic.h>
#include <builtinFonts/compact_notosans_11_regular.h>
#include <builtinFonts/compact_notosans_11_bold.h>
#include <builtinFonts/compact_notosans_11_italic.h>
#include <builtinFonts/compact_notosans_11_bolditalic.h>

// Register only the extra reader sizes 10 and 11 once during setup; static objects retain their lifetime for the renderer.
inline void registerCompactReaderFonts(GfxRenderer& renderer) {
  static EpdFont compact_notoserif_10_regularFont(&compact_notoserif_10_regular);
  static EpdFont compact_notoserif_10_boldFont(&compact_notoserif_10_bold);
  static EpdFont compact_notoserif_10_italicFont(&compact_notoserif_10_italic);
  static EpdFont compact_notoserif_10_bolditalicFont(&compact_notoserif_10_bolditalic);
  static EpdFontFamily compact_notoserif_10Family(&compact_notoserif_10_regularFont, &compact_notoserif_10_boldFont,
                                    &compact_notoserif_10_italicFont, &compact_notoserif_10_bolditalicFont);
  renderer.insertFont(COMPACT_NOTOSERIF_10_FONT_ID, compact_notoserif_10Family);
  static EpdFont compact_notoserif_11_regularFont(&compact_notoserif_11_regular);
  static EpdFont compact_notoserif_11_boldFont(&compact_notoserif_11_bold);
  static EpdFont compact_notoserif_11_italicFont(&compact_notoserif_11_italic);
  static EpdFont compact_notoserif_11_bolditalicFont(&compact_notoserif_11_bolditalic);
  static EpdFontFamily compact_notoserif_11Family(&compact_notoserif_11_regularFont, &compact_notoserif_11_boldFont,
                                    &compact_notoserif_11_italicFont, &compact_notoserif_11_bolditalicFont);
  renderer.insertFont(COMPACT_NOTOSERIF_11_FONT_ID, compact_notoserif_11Family);
  static EpdFont compact_notosans_10_regularFont(&compact_notosans_10_regular);
  static EpdFont compact_notosans_10_boldFont(&compact_notosans_10_bold);
  static EpdFont compact_notosans_10_italicFont(&compact_notosans_10_italic);
  static EpdFont compact_notosans_10_bolditalicFont(&compact_notosans_10_bolditalic);
  static EpdFontFamily compact_notosans_10Family(&compact_notosans_10_regularFont, &compact_notosans_10_boldFont,
                                    &compact_notosans_10_italicFont, &compact_notosans_10_bolditalicFont);
  renderer.insertFont(COMPACT_NOTOSANS_10_FONT_ID, compact_notosans_10Family);
  static EpdFont compact_notosans_11_regularFont(&compact_notosans_11_regular);
  static EpdFont compact_notosans_11_boldFont(&compact_notosans_11_bold);
  static EpdFont compact_notosans_11_italicFont(&compact_notosans_11_italic);
  static EpdFont compact_notosans_11_bolditalicFont(&compact_notosans_11_bolditalic);
  static EpdFontFamily compact_notosans_11Family(&compact_notosans_11_regularFont, &compact_notosans_11_boldFont,
                                    &compact_notosans_11_italicFont, &compact_notosans_11_bolditalicFont);
  renderer.insertFont(COMPACT_NOTOSANS_11_FONT_ID, compact_notosans_11Family);
}
