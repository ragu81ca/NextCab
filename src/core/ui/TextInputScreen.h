// TextInputScreen.h — Data-only model for text / numeric input screens.
//
// Shows 1-2 lines of centred prompt text, the current input value (centred,
// with a blinking caret at the end), and a footer with key instructions.
//
// The `frame` counter drives the caret blink — callers call advance() on a
// timer (e.g. every 125 ms) and re-render.  The Renderer toggles the caret
// visibility based on the frame count.
//
// Key principle: the screen knows NOTHING about pixels or fonts.
#pragma once

#include "IScreen.h"
#include <functional>

class TextInputScreen : public IScreen {
public:
    ScreenType type() const override { return ScreenType::TextInput; }

    // ── Callbacks ────────────────────────────────────────────────────────
    using CancelCallback = std::function<void()>;
    CancelCallback onCancel;                // Called when * pressed on empty input

    // ── Prompt ──────────────────────────────────────────────────────────
    String promptLine1;                     // First line of centred prompt
    String promptLine2;                     // Second line (optional)

    // ── Input ───────────────────────────────────────────────────────────
    String inputText;                       // Current value (caret is always at end)
    int    maxLength   = 0;                 // Maximum characters (0 = unlimited)
    bool   maskInput   = false;             // Show '*' instead of actual chars

    // ── Footer ──────────────────────────────────────────────────────────
    String footerText;                      // e.g. "# Submit  * Delete"

    // ── Animation ───────────────────────────────────────────────────────
    int  frame = 0;                         // Caller increments via advance()
    void advance() { frame++; }

    // ── Helpers ─────────────────────────────────────────────────────────

    /// Append a character (respects maxLength).
    /// Returns true if the character was added.
    bool addChar(char c) {
        if (maxLength > 0 && (int)inputText.length() >= maxLength) return false;
        inputText += c;
        return true;
    }

    /// Delete the last character.
    /// Returns true if a character was removed.
    bool backspace() {
        if (inputText.length() == 0) return false;
        inputText.remove(inputText.length() - 1);
        return true;
    }

    /// The display string — masked or plain.
    String displayText() const {
        if (!maskInput) return inputText;
        String masked;
        for (unsigned int i = 0; i < inputText.length(); i++) masked += '*';
        return masked;
    }

    void clear() {
        promptLine1 = "";
        promptLine2 = "";
        inputText   = "";
        maxLength   = 0;
        maskInput   = false;
        footerText  = "";
        frame       = 0;
        onCancel    = nullptr;
    }
};
