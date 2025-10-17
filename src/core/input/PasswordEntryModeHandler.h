#pragma once
#include "IModeHandler.h"
#include <Arduino.h>
// Legacy globals will be phased out; handler maintains its own state now.
extern const char ssidPasswordBlankChar; // still used for initial blank glyph in renderer until refactored fully

// Handles collection of password characters from keypad events until commit.
class PasswordEntryModeHandler : public IModeHandler {
public:
    using CommitCallback = void (* )();

    PasswordEntryModeHandler(size_t maxLen, CommitCallback onCommit = nullptr)
        : maxLen_(maxLen), commitCb_(onCommit) {}

    bool handle(const InputEvent &ev) override;
    void onEnter() override {
        buffer_.remove(0);
        previewChar_ = ssidPasswordBlankChar;
        currentIndex_ = 0;
        activeSelection_ = false;
        render();
    }
    void onExit() override { /* no-op */ }

    // Accessors for renderer / commit logic
    const String &entered() const { return buffer_; }
    char previewChar() const { return previewChar_; }

private:
    void render();
    String buffer_ {""};
    size_t maxLen_;
    CommitCallback commitCb_ { nullptr };

    // Encoder-driven character selection
    static constexpr const char *kCharSet =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%&()_-+=[]{}.,?"; // WiFi password superset (81 chars)
    static constexpr size_t kCharSetLen = 81; // update if kCharSet changes
    size_t currentIndex_ {0};
    bool activeSelection_ {false};
    char previewChar_ { ssidPasswordBlankChar };
};
