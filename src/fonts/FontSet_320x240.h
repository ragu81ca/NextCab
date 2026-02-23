// FontSet_320x240.h - Font profile for 320×240 color TFT display
// Uses larger U8g2 fonts to take advantage of the higher resolution.
// When a TFT-native driver is added, these would be replaced with TFT font mappings.
// For now these are placeholder U8g2 fonts that would work if U8g2 is used on TFT.
#pragma once
#include "../core/FontSet.h"
#include <U8g2lib.h>

// Larger fonts that better suit a 320×240 display
// These can be refined once the physical display is available for testing.
const FontSet FONTS_320x240 = {
    .defaultFont        = u8g2_font_helvR12_te,      // ~12px proportional
    .speed              = u8g2_font_logisoso42_tr,    // ~42px speed digits
    .direction          = u8g2_font_helvB14_tr,       // ~14px bold direction
    .password           = u8g2_font_courR14_tf,       // ~14px monospace password
    .throttleNumber     = u8g2_font_helvB14_tr,       // ~14px bold throttle #
    .functionIndicators = u8g2_font_helvR08_tr,       // ~8px function indicators
    .glyphs             = u8g2_font_open_iconic_all_2x_t, // 2x glyphs (16px)
    .nextThrottle       = u8g2_font_helvR10_tr,       // ~10px next throttle info
};
