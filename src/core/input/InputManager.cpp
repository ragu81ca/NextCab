#include "InputManager.h"
#include "InputEvents.h"

void InputManager::setMode(InputMode mode) {
    if (mode_ == mode) return;
    if (active_) active_->onExit();
    mode_ = mode;
    switch (mode_) {
        case InputMode::Operation: active_ = opHandler_; break;
        case InputMode::PasswordEntry: active_ = pwHandler_; break;
    }
    if (active_) active_->onEnter();
}

void InputManager::dispatch(const InputEvent &ev) {
    if (active_) {
        bool consumed = active_->handle(ev);
        if (consumed) return;
    }
    // Fallbacks (global shortcuts) could be handled here later.
}
