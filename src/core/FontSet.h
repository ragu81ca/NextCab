// FontSet.h - Font profile for display-agnostic rendering
// Each display profile provides a FontSet mapping logical font roles to concrete font pointers.
#pragma once
#include <Arduino.h>

struct FontSet {
    const uint8_t* defaultFont;           // Normal text (menu items, labels)
    const uint8_t* speed;                 // Large speed number (OLED size)
    const uint8_t* speedLarge;            // Extra-large speed number (TFT size, same as speed on OLED)
    const uint8_t* direction;             // Forward/Reverse text
    const uint8_t* password;              // Password entry (monospace)
    const uint8_t* throttleNumber;        // Throttle index number
    const uint8_t* functionIndicators;    // Tiny function state numbers
    const uint8_t* glyphs;               // Icon glyphs (track power, heartbeat, speed step)
    const uint8_t* batteryGlyphs;         // Battery icon glyph (may be smaller than glyphs on TFT)
    const uint8_t* nextThrottle;          // Next throttle info (symbols font)
};
