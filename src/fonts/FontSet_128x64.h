// FontSet_128x64.h - Font profile for 128×64 monochrome OLED
// Maps to the exact same U8g2 fonts that were previously hardcoded in static.h
#pragma once
#include "../core/FontSet.h"
#include <U8g2lib.h>

// These match the original FONT_* defines from static.h
const FontSet FONTS_128x64 = {
    .defaultFont        = u8g2_font_NokiaSmallPlain_te,
    .speed              = u8g2_font_profont29_mr,
    .speedLarge         = u8g2_font_profont29_mr,       // same as speed on small display
    .direction          = u8g2_font_neuecraft_tr,
    .password           = u8g2_font_9x15_tf,
    .throttleNumber     = u8g2_font_neuecraft_tr,
    .functionIndicators = u8g2_font_tiny_simon_tr,
    .glyphs             = u8g2_font_open_iconic_all_1x_t,
    .batteryGlyphs      = u8g2_font_open_iconic_all_1x_t,
    .nextThrottle       = u8g2_font_6x12_m_symbols,
};
