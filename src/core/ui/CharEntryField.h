// CharEntryField.h — Text buffer with encoder-driven character selection.
//
// The keypad can only produce digits, so alphabetic input is entered by
// rotating the encoder through a character set and confirming with a click.
// The candidate character is held outside the buffer until confirmed, so the
// caller can render it as a distinct preview.
//
// Event-agnostic by design: callers translate their own input events into
// these calls, which keeps the field testable and reusable across screens.
#pragma once

#include <Arduino.h>
#include "TextInputScreen.h"

class CharEntryField {
public:
    enum class BackspaceResult : uint8_t {
        PreviewCancelled,  // pending preview discarded, buffer untouched
        CharRemoved,
        Empty              // nothing left to remove — caller usually cancels
    };

    void begin(const char *charSet, size_t charSetLen, size_t maxLen,
               const String &initial = String()) {
        charSet_         = charSet;
        charSetLen_      = charSetLen;
        maxLen_          = maxLen;
        buffer_          = initial;
        currentIndex_    = 0;
        activeSelection_ = false;
        previewChar_     = 0;
    }

    /// Move the candidate character by delta, activating the preview.
    void cycle(int delta) {
        if (charSetLen_ == 0 || delta == 0) return;
        activeSelection_ = true;
        long next = (long)currentIndex_ + (long)delta;
        while (next < 0) next += (long)charSetLen_;
        while ((size_t)next >= charSetLen_) next -= (long)charSetLen_;
        currentIndex_ = (size_t)next;
        previewChar_  = charSet_[currentIndex_];
    }

    /// Append the previewed character. False when no preview was pending,
    /// which lets the caller pass the click on to another handler.
    bool commitPreview() {
        if (!activeSelection_) return false;
        if (buffer_.length() < maxLen_) buffer_ += charSet_[currentIndex_];
        activeSelection_ = false;
        previewChar_     = 0;
        return true;
    }

    /// Append a directly-typed character, confirming any pending preview first.
    void addChar(char c) {
        if (buffer_.length() >= maxLen_) return;
        commitPreview();
        if (buffer_.length() < maxLen_) buffer_ += c;
    }

    BackspaceResult backspace() {
        if (activeSelection_) {
            activeSelection_ = false;
            previewChar_     = 0;
            return BackspaceResult::PreviewCancelled;
        }
        if (buffer_.length() > 0) {
            buffer_.remove(buffer_.length() - 1);
            return BackspaceResult::CharRemoved;
        }
        return BackspaceResult::Empty;
    }

    const String &value() const { return buffer_; }

    /// Write the buffer and preview/caret state onto a screen model.
    void applyTo(TextInputScreen &screen) const {
        screen.inputText = buffer_;
        if (activeSelection_ && previewChar_ != 0) {
            screen.inputText += previewChar_;
            screen.highlightPos = (int)buffer_.length();
        } else {
            screen.highlightPos = -1;
        }
    }

private:
    const char *charSet_    { nullptr };
    size_t      charSetLen_ { 0 };
    size_t      maxLen_     { 0 };
    String      buffer_;
    size_t      currentIndex_    { 0 };
    bool        activeSelection_ { false };
    char        previewChar_     { 0 };
};
