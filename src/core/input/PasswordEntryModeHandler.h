#pragma once
#include "IModeHandler.h"
#include "../ui/TextInputScreen.h"
#include <Arduino.h>

class Renderer;

// Handles collection of password characters from keypad events until commit.
class PasswordEntryModeHandler : public IModeHandler {
public:
    using CommitCallback = void (* )();

    PasswordEntryModeHandler(size_t maxLen, CommitCallback onCommit = nullptr)
        : maxLen_(maxLen), commitCb_(onCommit) {}

    /// Set a callback invoked when the user presses * on an empty input (cancel).
    void setCancelCallback(TextInputScreen::CancelCallback cb) { cancelCb_ = cb; }

    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override { /* no-op */ }

    // Accessors for commit logic
    const String &entered() const { return buffer_; }

    /// Called periodically (e.g. every 125 ms) to advance the caret animation.
    void tick();

private:
    void render();
    String buffer_ {""};
    size_t maxLen_;
    CommitCallback commitCb_ { nullptr };
    TextInputScreen::CancelCallback cancelCb_;

    // Encoder-driven character selection
    static constexpr const char *kCharSet =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%&()_-+=[]{}.,?"; // WiFi password superset (81 chars)
    static constexpr size_t kCharSetLen = 81; // update if kCharSet changes
    size_t currentIndex_ {0};
    bool activeSelection_ {false};
    char previewChar_ {0};

    TextInputScreen screen_;
};
