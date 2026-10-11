#pragma once

#include "fontIds.h"

// Additional built-in Ubuntu UI sizes; keep generated stock fontIds.h unchanged.
// IDs use the same SHA-256 family-hash rule as build-font-ids.sh.
#define UI_6_FONT_ID (490505600)
#define UI_7_FONT_ID (-106000525)
#define UI_8_FONT_ID (-833236886)
#define UI_9_FONT_ID (1394241723)

static_assert(UI_6_FONT_ID != 0, "Custom UI font ID must not be zero");
static_assert(UI_7_FONT_ID != 0, "Custom UI font ID must not be zero");
static_assert(UI_8_FONT_ID != 0, "Custom UI font ID must not be zero");
static_assert(UI_9_FONT_ID != 0, "Custom UI font ID must not be zero");
static_assert(UI_6_FONT_ID != UI_10_FONT_ID, "Custom UI font ID collision");
static_assert(UI_6_FONT_ID != UI_12_FONT_ID, "Custom UI font ID collision");
static_assert(UI_6_FONT_ID != SMALL_FONT_ID, "Custom UI font ID collision");
static_assert(UI_7_FONT_ID != UI_10_FONT_ID, "Custom UI font ID collision");
static_assert(UI_7_FONT_ID != UI_12_FONT_ID, "Custom UI font ID collision");
static_assert(UI_7_FONT_ID != SMALL_FONT_ID, "Custom UI font ID collision");
static_assert(UI_8_FONT_ID != UI_10_FONT_ID, "Custom UI font ID collision");
static_assert(UI_8_FONT_ID != UI_12_FONT_ID, "Custom UI font ID collision");
static_assert(UI_8_FONT_ID != SMALL_FONT_ID, "Custom UI font ID collision");
static_assert(UI_9_FONT_ID != UI_10_FONT_ID, "Custom UI font ID collision");
static_assert(UI_9_FONT_ID != UI_12_FONT_ID, "Custom UI font ID collision");
static_assert(UI_9_FONT_ID != SMALL_FONT_ID, "Custom UI font ID collision");
