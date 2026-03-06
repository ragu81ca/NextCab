// ColorPalette.h — Centralised RGB565 colour constants and state→colour helpers.
//
// All UI colour decisions live here so they can be tuned in one place.
// Colours are stored as RGB565 (uint16_t) for direct use on TFT displays.
// Monochrome (OLED) code ignores these — colour is only applied when
// colorDepth() >= 16.
#pragma once
#include <Arduino.h>

// ── Named palette (RGB565) ──────────────────────────────────────────────
// Source hex  → RGB565 conversion (R5 G6 B5)
//   #6EEB83  → 0x6F50       #FFD166 → 0xFE8C       #FF5A5F → 0xFACB
//   #4DA3FF  → 0x4D1F       #B0B7C3 → 0xB5B8       #F5F7FA → 0xF7BF
static constexpr uint16_t COLOR_GREEN      = 0x6F50;  // #6EEB83  — positive status
static constexpr uint16_t COLOR_YELLOW     = 0xFE8C;  // #FFD166  — caution / ramping
static constexpr uint16_t COLOR_RED        = 0xFACB;  // #FF5A5F  — brake / warning
static constexpr uint16_t COLOR_BLUE       = 0x4D1F;  // #4DA3FF  — accent highlight
static constexpr uint16_t COLOR_LIGHT_GREY = 0xB5B8;  // #B0B7C3  — secondary text
static constexpr uint16_t COLOR_DIM_GREY   = 0x4228;  // #404850  — receded elements (dividers, inactive)
static constexpr uint16_t COLOR_MID_GREY   = 0x7BCF;  // #7B7D78  — secondary icons
static constexpr uint16_t COLOR_WHITE      = 0xFFFF;  // pure white (default text)
static constexpr uint16_t COLOR_BLACK      = 0x0000;

// ── State → colour helpers ──────────────────────────────────────────────

/// Return the speed-text colour based on the current brake/momentum state.
///   brakeState:  0 = idle, 1 = braking, 2 = ramping up, 3 = ramping down
/// Priority: brake (RED) > momentum ramp (YELLOW) > normal (GREEN).
inline uint16_t resolveSpeedColor(int brakeState) {
    if (brakeState == 1) return COLOR_RED;
    if (brakeState == 2 || brakeState == 3) return COLOR_YELLOW;
    return COLOR_WHITE;   // normal state: no color — white is the baseline
}

/// Return the battery-percentage text colour.
///   pct: battery percentage 0-100
inline uint16_t resolveBatteryColor(int pct) {
    if (pct < 10)  return COLOR_RED;
    if (pct <= 25) return COLOR_YELLOW;
    return COLOR_GREEN;
}
