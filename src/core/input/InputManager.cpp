#include "InputManager.h"
#include "InputEvents.h"
// For loco-centric emergency actions when not in Operation mode
#include "OperationModeHandler.h"
#include "../../../actions.h"
extern OperationModeHandler operationModeHandler; // defined in sketch


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
    // AdditionalButton events now represent canonical function button state changes:
    // cvalue 'P' => press (turn function on for toggle or start for momentary)
    // cvalue 'R' => release (only for momentary functions)
    if (ev.type == InputEventType::AdditionalButton) {
        bool pressed = (ev.cvalue == 'P');
        int buttonIndex = ev.ivalue;
        extern int additionalButtonActions[]; // from sketch
        int actionCode = additionalButtonActions[buttonIndex];
        if (actionCode == FUNCTION_NULL) return;
        if (actionCode >= FUNCTION_0 && actionCode <= FUNCTION_31) {
            // Directly invoke function control for current throttle index.
            extern class ThrottleManager throttleManager;
            int mtIndex = throttleManager.getCurrentThrottleIndex();
            throttleManager.directFunction(mtIndex, actionCode, pressed);
            return;
        } else {
            if (!pressed) return; // non-function actions fire on press only
            InputEvent actEv; actEv.timestamp = ev.timestamp; actEv.type = InputEventType::Action; actEv.ivalue = actionCode; actEv.cvalue = 0;
            if (active_) { bool consumed = active_->handle(actEv); if (consumed) return; }
            if (actionHandler_) actionHandler_->handle(actEv);
            return;
        }
    }
    // Generic action fallback: route Action events to actionHandler_ when not consumed.
    if (ev.type == InputEventType::Action && actionHandler_) {
        actionHandler_->handle(ev);
        return;
    }
    // Other global shortcuts could be added here.
}
