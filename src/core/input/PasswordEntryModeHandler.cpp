#include "PasswordEntryModeHandler.h"
#include "../Renderer.h"
#include "static.h" // root-level include (platformio include_dir=.)

extern Renderer renderer;

void PasswordEntryModeHandler::onEnter() {
    buffer_ = "";
    previewChar_ = 0;
    currentIndex_ = 0;
    activeSelection_ = false;

    screen_.clear();
    screen_.promptLine1 = MSG_ENTER_PASSWORD;
    screen_.footerText  = menu_text[menu_enter_ssid_password];
    screen_.maxLength   = 0; // we manage length ourselves (encoder preview complicates it)
    screen_.onCancel    = cancelCb_;
    render();
}

void PasswordEntryModeHandler::render() {
    // Build the display text: entered buffer + encoder preview char (if cycling)
    screen_.inputText = buffer_;
    if (activeSelection_ && previewChar_ != 0) {
        screen_.inputText += previewChar_;
        screen_.highlightPos = (int)buffer_.length(); // highlight the preview char
    } else {
        screen_.highlightPos = -1; // show blinking cursor line
    }
    renderer.renderTextInput(screen_);
}

void PasswordEntryModeHandler::tick() {
    screen_.advance();
    render();
}

bool PasswordEntryModeHandler::handle(const InputEvent &ev) {
    switch (ev.type) {
        case InputEventType::SpeedDelta: {
            // Use encoder rotation to cycle candidate characters (does not modify buffer until click)
            if (kCharSetLen == 0) return true;
            int delta = ev.ivalue; // may be negative
            if (delta == 0) return true;
            // Initialize selection if not yet active
            if (!activeSelection_) {
                activeSelection_ = true;
                // currentIndex_ already points to last used (or 0 at start)
            }
            long next = (long)currentIndex_ + (long)delta;
            // wrap safely
            while (next < 0) next += kCharSetLen;
            while ((size_t)next >= kCharSetLen) next -= kCharSetLen;
            currentIndex_ = (size_t)next;
            previewChar_ = kCharSet[currentIndex_];
            render();
            return true;
        }
        case InputEventType::EncoderClick: {
            // Commit currently previewed char to buffer (if any)
            if (activeSelection_) {
                if (buffer_.length() < maxLen_) {
                    buffer_ += kCharSet[currentIndex_];
                }
                // Deactivate selection so the committed char isn't shown
                // as a duplicate preview, and won't be re-added on '#' submit.
                activeSelection_ = false;
                previewChar_ = 0;
                render();
                return true;
            }
            return false; // let other handlers use click if no selection yet
        }
        case InputEventType::KeypadChar: {
            if (buffer_.length() < maxLen_) {
                // If encoder preview was active, commit it first
                if (activeSelection_) {
                    buffer_ += kCharSet[currentIndex_];
                    activeSelection_ = false;
                    previewChar_ = 0;
                }
                if (buffer_.length() < maxLen_) {
                    buffer_ += ev.cvalue;
                }
                render();
            }
            return true;
        }
        case InputEventType::KeypadSpecial: {
            // Treat '*' as backspace and '#' as commit
            if (ev.cvalue == '*') {
                if (activeSelection_) {
                    // Cancel the encoder preview without deleting
                    activeSelection_ = false;
                    previewChar_ = 0;
                    render();
                } else if (buffer_.length() > 0) {
                    buffer_.remove(buffer_.length()-1);
                    render();
                } else {
                    // Empty buffer + back = cancel
                    if (screen_.onCancel) screen_.onCancel();
                }
                return true;
            } else if (ev.cvalue == '#') {
                // Commit any pending encoder preview before submitting
                if (activeSelection_ && buffer_.length() < maxLen_) {
                    buffer_ += kCharSet[currentIndex_];
                    activeSelection_ = false;
                    previewChar_ = 0;
                }
                if (commitCb_) commitCb_();
                return true;
            } else {
                return false;
            }
        }
        case InputEventType::KeypadCharRelease:
        case InputEventType::KeypadSpecialRelease: {
            return true;
        }
        case InputEventType::PasswordCommit: {
            if (commitCb_) commitCb_();
            return true;
        }
        default:
            return false;
    }
}
