#pragma once
#include "IModeHandler.h"
#include <Arduino.h>
// Forward declare password UI globals (defined in sketch/static)
extern char ssidPasswordCurrentChar;
extern const char ssidPasswordBlankChar;

// Handles collection of password characters from keypad events until commit.
class PasswordEntryModeHandler : public IModeHandler {
public:
    using CommitCallback = void (*)();

    PasswordEntryModeHandler(String &targetBuffer, size_t maxLen, CommitCallback onCommit = nullptr)
        : buffer_(targetBuffer), maxLen_(maxLen), commitCb_(onCommit) {}

    bool handle(const InputEvent &ev) override;
    void onEnter() override {
        buffer_.remove(0);
        // Reset selection state (preview char hidden until user rotates encoder)
        currentIndex_ = 0;
        activeSelection_ = false;
        ssidPasswordCurrentChar = ssidPasswordBlankChar;
        render();
    }
    void onExit() override { /* no-op */ }

private:
    void render();
    String &buffer_;
    size_t maxLen_;
    CommitCallback commitCb_ { nullptr };

    // Encoder-driven character selection
    static constexpr const char *kCharSet =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%&()_-+=[]{}.,?"; // WiFi password superset (81 chars)
    static constexpr size_t kCharSetLen = 81; // update if kCharSet changes
    size_t currentIndex_ {0};
    bool activeSelection_ {false};
};
