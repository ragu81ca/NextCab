// ThrottleScreen.h — Data-only model for the main throttle operating screen.
//
// Captures everything the Renderer needs to draw the operating display:
// loco identity (single or consist), speed, direction, function states,
// next-throttle preview, and system status indicators.
//
// Key principle: the screen knows NOTHING about pixels or fonts.
// All data is pre-formatted by the caller; the Renderer only lays it out.
#pragma once

#include "IScreen.h"
#include "../../../static.h"   // MAX_FUNCTIONS, Direction, TrackPower

class ThrottleScreen : public IScreen {
public:
    // ── IScreen ─────────────────────────────────────────────────────────
    ScreenType type() const override { return ScreenType::Operating; }

    // ── Consist / loco identity ─────────────────────────────────────────
    // A throttle controls one or more locomotives.  The display shows a
    // summary string for the header line, plus per-loco detail when
    // editing the consist.

    /// Display-ready string for the header line.
    /// Single loco:  "1234" or "UP Big Boy"
    /// Consist:      "1234 5678(R) 9012"
    /// Built by the caller using roster lookups and reverse indicators.
    String locoDisplayString;

    /// Number of locomotives on this throttle (0 = no loco selected).
    int locoCount = 0;

    // ── Speed & direction ───────────────────────────────────────────────
    /// Display-formatted speed string (e.g. "42", "126", "0").
    /// Already scaled per the current speed-step display mode.
    String speedDisplay;

    /// Direction text for large-font display (e.g. "Fwd", "Rev").
    String directionDisplay;

    // ── Multi-throttle ──────────────────────────────────────────────────
    /// 1-based throttle number for display (e.g. 1, 2, 3).
    int throttleNumber = 1;

    /// Total active throttles (if > 1, show throttle number badge).
    int maxThrottles = 1;

    // ── Next throttle preview ───────────────────────────────────────────
    /// True when there's another throttle with a loco to preview.
    bool hasNextThrottle = false;

    /// 1-based number of the next active throttle.
    String nextThrottleNumber;

    /// Compact speed+direction string for the preview, e.g. "  42>" or "<  0".
    String nextThrottleSpeedDir;

    // ── Function states ─────────────────────────────────────────────────
    /// Per-function active state (for indicator boxes in the status bar).
    bool functionActive[MAX_FUNCTIONS] = {};

    // ── System status indicators ────────────────────────────────────────
    TrackPower trackPower = PowerUnknown;
    bool heartbeatEnabled = true;

    /// Current speed step value (shown as indicator when different from default).
    int currentSpeedStep = 1;

    /// Momentum level: 0=Off, 1=Low, 2=Med, 3=High.
    int momentumLevel = 0;

    /// Brake / ramp state for the momentum arrow indicator:
    ///   0 = idle (no indicator)
    ///   1 = braking (red tint on TFT, inverted box on OLED)
    ///   2 = ramping up (progress bar)
    ///   3 = ramping down (progress bar)
    int brakeState = 0;

    /// Service brake active — direction indicator shows "Brk" instead of Fwd/Rev.
    bool serviceBrakeActive = false;

    /// Actual speed (momentum-smoothed) for ramp progress bar.
    int actualSpeed = 0;

    /// Target speed for ramp progress bar.
    int targetSpeed = 0;

    // ── Menu text ───────────────────────────────────────────────────────
    /// Footer/menu-bar text lines (rendered in the bottom status area).
    /// These carry the key-action labels (e.g. "# Fn  0 Spd  * Menu").
    static constexpr int MAX_MENU_LINES = 18;
    String menuLines[MAX_MENU_LINES];
    bool   menuInvert[MAX_MENU_LINES] = {};

    // ── Convenience ─────────────────────────────────────────────────────
    bool hasLoco() const { return locoCount > 0; }

    void clear() {
        locoDisplayString = "";
        locoCount          = 0;
        speedDisplay       = "";
        directionDisplay   = "";
        throttleNumber     = 1;
        maxThrottles       = 1;
        hasNextThrottle    = false;
        nextThrottleNumber = "";
        nextThrottleSpeedDir = "";
        for (int i = 0; i < MAX_FUNCTIONS; i++) functionActive[i] = false;
        trackPower         = PowerUnknown;
        heartbeatEnabled   = true;
        currentSpeedStep    = 1;
        momentumLevel      = 0;
        brakeState         = 0;
        serviceBrakeActive = false;
        actualSpeed        = 0;
        targetSpeed        = 0;
        for (int i = 0; i < MAX_MENU_LINES; i++) {
            menuLines[i]  = "";
            menuInvert[i] = false;
        }
    }
};
