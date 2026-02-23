// DisplayConfig.h - Compile-time display profile selector
// Include this header to get the active DisplayLayout, FontSet, and DisplayDriver type.
//
// Users select their display by defining DISPLAY_PROFILE_128x64 or DISPLAY_PROFILE_320x240
// in config_buttons.h. If nothing is defined, defaults to 128×64.
#pragma once

// Default to 128×64 if no profile is selected
#if !defined(DISPLAY_PROFILE_128x64) && !defined(DISPLAY_PROFILE_320x240)
  #define DISPLAY_PROFILE_128x64
#endif

// ── 128×64 Monochrome OLED (default) ──
#ifdef DISPLAY_PROFILE_128x64

  #include "../layouts/Layout_128x64.h"
  #include "../fonts/FontSet_128x64.h"
  #include "../drivers/DisplayDriver_U8g2.h"

  #define ACTIVE_LAYOUT    LAYOUT_128x64
  #define ACTIVE_FONTS     FONTS_128x64
  // DisplayDriver type: DisplayDriver_U8g2 (wraps U8G2)

#endif

// ── 320×240 Color TFT ──
#ifdef DISPLAY_PROFILE_320x240

  #include "../layouts/Layout_320x240.h"
  #include "../fonts/FontSet_320x240.h"
  // For now, still use U8g2 driver — a TFT_eSPI driver can be added later
  #include "../drivers/DisplayDriver_U8g2.h"

  #define ACTIVE_LAYOUT    LAYOUT_320x240
  #define ACTIVE_FONTS     FONTS_320x240
  // DisplayDriver type: DisplayDriver_U8g2 (or DisplayDriver_TFT when implemented)

#endif

// Provide the active layout and fonts as extern references for code that needs them
// (The actual instances are defined in display_init.cpp)
#include "../core/DisplayLayout.h"
#include "../core/FontSet.h"
#include "../core/DisplayDriver.h"

extern const DisplayLayout& activeLayout;
extern const FontSet& activeFonts;
extern DisplayDriver& displayDriver;
