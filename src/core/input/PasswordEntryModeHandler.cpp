#include "PasswordEntryModeHandler.h"
#include "../OledRenderer.h"
#include "static.h" // root-level include (platformio include_dir=.)

extern OledRenderer oledRenderer;
extern int encoderUseType; // transitional

void PasswordEntryModeHandler::render() {
    // Temporary bridge: update legacy globals consumed by renderer until renderer is refactored.
    // Remove when OledRenderer pulls from handler directly.
    extern String ssidPasswordEntered; // legacy global
    extern char ssidPasswordCurrentChar; // legacy global preview
    ssidPasswordEntered = buffer_; // keep global in sync for now
    ssidPasswordCurrentChar = previewChar_;
    oledRenderer.renderEnterPassword();
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
                // Keep preview char so user can continue cycling for next char
                render();
                return true;
            }
            return false; // let other handlers use click if no selection yet
        }
        case InputEventType::KeypadChar: {
            if (buffer_.length() < maxLen_) {
                buffer_ += ev.cvalue;
                render();
            }
            return true;
        }
        case InputEventType::KeypadSpecial: {
            // Treat '*' as backspace and '#' as commit if present
            if (ev.cvalue == '*') {
                if (buffer_.length() > 0) {
                    buffer_.remove(buffer_.length()-1);
                    // After deletion, keep current preview (do not alter activeSelection_)
                    render();
                }
                return true;
            } else if (ev.cvalue == '#') {
                if (commitCb_) commitCb_();
                return true;
            } else {
                // ignore other specials
                return false;
            }
        }
        case InputEventType::KeypadCharRelease:
        case InputEventType::KeypadSpecialRelease: {
            // Releases are ignored in password entry (press already consumed)
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
