// RadioSelectScreen.h — Data-only model for single-choice selection screens.
//
// Shows a title, a short vertical list of labelled options centred on screen,
// with the current selection highlighted.  The user rotates the encoder to
// move the highlight up/down, and presses '#' (or encoder click) to confirm
// or '*' to cancel.
//
// Designed for 3–8 options — loco type, yes/no toggles, etc.
//
// Key principle: the screen knows NOTHING about pixels or fonts.
#pragma once

#include "IScreen.h"
#include <Arduino.h>
#include <functional>

class RadioSelectScreen : public IScreen {
public:
    static constexpr int MAX_OPTIONS = 9;

    // ── IScreen ─────────────────────────────────────────────────────────
    ScreenType type() const override { return ScreenType::RadioSelect; }

    // ── Callbacks ───────────────────────────────────────────────────────
    /// Called when the user confirms (# key or encoder click).
    using ConfirmFn = std::function<void(int selectedIndex)>;

    /// Called when the user cancels (* key).
    using CancelFn  = std::function<void()>;

    ConfirmFn onConfirm;
    CancelFn  onCancel;

    // ── Content ─────────────────────────────────────────────────────────
    String title;                           // Header line (e.g. "Loco Type")
    String options[MAX_OPTIONS];            // Display labels for each option
    int    optionCount    = 0;              // Number of valid options (1–9)
    int    selectedIndex  = 0;              // Currently highlighted option (0-based)
    String footerText     = "# OK  * Cancel";

    // ── Helpers ─────────────────────────────────────────────────────────

    /// Add an option to the list.  Returns the 0-based index, or -1 if full.
    int addOption(const String &label) {
        if (optionCount >= MAX_OPTIONS) return -1;
        options[optionCount] = label;
        return optionCount++;
    }

    /// Move selection up (negative delta) or down (positive delta), clamping to bounds.
    void moveSelection(int delta) {
        int next = selectedIndex + delta;
        if (next < 0) next = 0;
        if (next >= optionCount) next = optionCount - 1;
        selectedIndex = next;
    }

    /// Reset to empty state.
    void reset() {
        title = "";
        for (int i = 0; i < MAX_OPTIONS; i++) options[i] = "";
        optionCount   = 0;
        selectedIndex = 0;
        footerText    = "# OK  * Cancel";
        onConfirm     = nullptr;
        onCancel      = nullptr;
    }
};
