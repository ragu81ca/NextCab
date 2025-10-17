#include "InputManager.h"
#include "InputEvents.h"

void InputManager::setMode(InputMode mode) {
    if (mode_ == mode) {
        // Ensure active_ is correctly pointing in case handlers were set after initial mode assignment.
        active_ = resolveHandlerForMode(mode_);
        return; // treat as idempotent without re-calling onEnter
    }
    if (active_) active_->onExit();
    mode_ = mode;
    active_ = resolveHandlerForMode(mode_);
    if (active_) active_->onEnter();
}

void InputManager::forceMode(InputMode mode) {
    if (active_) active_->onExit();
    mode_ = mode;
    active_ = resolveHandlerForMode(mode_);
    if (active_) active_->onEnter();
}

void InputManager::dispatch(const InputEvent &ev) {
    if (active_) {
        bool consumed = active_->handle(ev);
        if (consumed) return;
    }
    // Generic action fallback: route Action events to actionHandler_ when not consumed.
    if (ev.type == InputEventType::Action && actionHandler_) {
        actionHandler_->handle(ev);
        return;
    }
    // Other global shortcuts could be added here.
}
