#include "PasswordEntryModeHandler.h"
#include "../Renderer.h"
#include "../../../static.h"

extern Renderer renderer;

void PasswordEntryModeHandler::onEnter() {
    field_.begin(kCharSet, kCharSetLen, maxLen_);

    screen_.clear();
    screen_.promptLine1 = MSG_ENTER_PASSWORD;
    screen_.footerText  = menu_text[menu_enter_ssid_password];
    screen_.maxLength   = 0; // we manage length ourselves (encoder preview complicates it)
    screen_.onCancel    = cancelCb_;
    render();
}

void PasswordEntryModeHandler::render() {
    field_.applyTo(screen_);
    renderer.renderTextInput(screen_);
}

void PasswordEntryModeHandler::tick() {
    screen_.advance();
    render();
}

bool PasswordEntryModeHandler::handle(const InputEvent &ev) {
    switch (ev.type) {
        case InputEventType::SpeedDelta: {
            // Encoder rotation cycles the candidate character without touching the buffer
            field_.cycle(ev.ivalue);
            render();
            return true;
        }
        case InputEventType::EncoderClick: {
            if (field_.commitPreview()) {
                render();
                return true;
            }
            return false; // let other handlers use click if no selection yet
        }
        case InputEventType::KeypadChar: {
            field_.addChar(ev.cvalue);
            render();
            return true;
        }
        case InputEventType::KeypadSpecial: {
            // Treat '*' as backspace and '#' as commit
            if (ev.cvalue == '*') {
                if (field_.backspace() == CharEntryField::BackspaceResult::Empty) {
                    if (screen_.onCancel) screen_.onCancel();
                } else {
                    render();
                }
                return true;
            } else if (ev.cvalue == '#') {
                field_.commitPreview();
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
