// FontSet.h - Font profile for display-agnostic rendering
// Each display profile provides a FontSet mapping logical font roles to concrete font pointers.
#pragma once
#include <Arduino.h>

struct FontSet {
    const uint8_t* defaultFont;           // Normal text (menu items, labels)
    const uint8_t* speed;                 // Large speed number
    const uint8_t* direction;             // Forward/Reverse text
    const uint8_t* password;              // Password entry (monospace)
    const uint8_t* throttleNumber;        // Throttle index number
    const uint8_t* functionIndicators;    // Tiny function state numbers
    const uint8_t* glyphs;               // Icon glyphs (battery, track power, etc.)
    const uint8_t* nextThrottle;          // Next throttle info (symbols font)
};
