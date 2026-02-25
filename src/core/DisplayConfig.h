// DisplayConfig.h - Compile-time display profile selector
// Include this header to get the active DisplayLayout, FontSet, and DisplayDriver type.
//
// Users select their display by defining DISPLAY_PROFILE_128x64 or DISPLAY_PROFILE_320x240
// in config_buttons.h (or via build_flags in platformio.ini).
// If nothing is defined, defaults to 128×64.
//
// When USE_TFT_ESPI is defined (typically via platformio.ini build_flags for TFT boards),
// the 320×240 profile will use TFT_eSPI + U8g2_for_TFT_eSPI instead of U8g2.
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

  #ifdef USE_TFT_ESPI
    // TFT_eSPI driver for SPI-connected color TFT (ILI9341, ST7789, etc.)
    #include "../drivers/DisplayDriver_TFT_eSPI.h"
  #else
    // Fallback: use U8g2 driver (e.g. for large U8g2-compatible panels)
    #include "../drivers/DisplayDriver_U8g2.h"
  #endif

  #define ACTIVE_LAYOUT    LAYOUT_320x240
  #define ACTIVE_FONTS     FONTS_320x240

#endif

// Provide the active layout and fonts as extern references for code that needs them
// (The actual instances are defined in display_init.cpp)
#include "../core/DisplayLayout.h"
#include "../core/FontSet.h"
#include "../core/DisplayDriver.h"

extern const DisplayLayout& activeLayout;
extern const FontSet& activeFonts;
extern DisplayDriver& displayDriver;
