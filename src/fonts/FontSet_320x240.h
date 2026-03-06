// FontSet_320x240.h - Font profile for 320×240 color TFT display
// Uses larger U8g2 fonts to take advantage of the higher resolution.
// When USE_TFT_ESPI is defined, we provide manual extern declarations for the font
// byte arrays to avoid including full U8g2lib.h (which would conflict with
// U8g2_for_TFT_eSPI.h's own internal struct definitions).
#pragma once
#include "../core/FontSet.h"

#ifdef USE_TFT_ESPI
  // Font data arrays are compiled from the U8g2 library sources; we only need
  // extern declarations here (no U8g2lib.h header, which would clash with
  // U8g2_for_TFT_eSPI.h).
  extern "C" {
    extern const uint8_t u8g2_font_helvR12_te[];
    extern const uint8_t u8g2_font_logisoso42_tr[];
    extern const uint8_t u8g2_font_logisoso78_tn[];
    extern const uint8_t u8g2_font_helvB14_tr[];
    extern const uint8_t u8g2_font_helvB18_tr[];
    extern const uint8_t u8g2_font_helvB24_tr[];
    extern const uint8_t u8g2_font_courR14_tf[];
    extern const uint8_t u8g2_font_helvR08_tr[];
    extern const uint8_t u8g2_font_open_iconic_all_2x_t[];
    extern const uint8_t u8g2_font_open_iconic_all_4x_t[];
    extern const uint8_t u8g2_font_helvR10_tr[];
  }
#else
  #include <U8g2lib.h>
#endif

// Larger fonts that better suit a 320×240 display
// These can be refined once the physical display is available for testing.
const FontSet FONTS_320x240 = {
    .defaultFont        = u8g2_font_helvR12_te,      // ~12px proportional
    .speed              = u8g2_font_logisoso42_tr,    // ~42px speed digits (menus etc.)
    .speedLarge         = u8g2_font_logisoso78_tn,    // ~78px speed digits (throttle screen)
    .direction          = u8g2_font_helvB24_tr,       // ~24px bold direction
    .password           = u8g2_font_courR14_tf,       // ~14px monospace password
    .throttleNumber     = u8g2_font_helvB14_tr,       // ~14px bold throttle #
    .functionIndicators = u8g2_font_helvR10_tr,       // ~10px function indicators (50% larger)
    .glyphs             = u8g2_font_open_iconic_all_4x_t, // 4x glyphs (32px)
    .batteryGlyphs      = u8g2_font_open_iconic_all_2x_t, // 2x for battery (16px, fits top bar)
    .nextThrottle       = u8g2_font_helvR10_tr,       // ~10px next throttle info
};
