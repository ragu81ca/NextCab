#include "InputManager.h"
#include "InputEvents.h"
// For loco-centric emergency actions when not in Operation mode
#include "OperationModeHandler.h"
#include "../../../actions.h"
extern OperationModeHandler operationModeHandler; // defined in sketch
extern void doDirectAdditionalButtonCommand(int, bool); // legacy function handler in sketch


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
    // Ensure critical loco-centric safety actions (E_STOP / current loco) are always processed even outside Operation mode.
    if (ev.type == InputEventType::Action && (ev.ivalue == E_STOP || ev.ivalue == E_STOP_CURRENT_LOCO)) {
        bool handled = operationModeHandler.handle(ev);
        if (handled) return;
    }
    // AdditionalButton events (function buttons) use legacy handler for latching semantics.
    if (ev.type == InputEventType::AdditionalButton) {
        bool pressed = (ev.cvalue == 'P');
        doDirectAdditionalButtonCommand(ev.ivalue, pressed);
        return;
    }
    // Generic action fallback: route Action events to actionHandler_ when not consumed.
    if (ev.type == InputEventType::Action && actionHandler_) {
        actionHandler_->handle(ev);
        return;
    }
    // Other global shortcuts could be added here.
}
