#include "OperationModeHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../ThrottleManager.h"
#include "../menu/MenuSystem.h"
#include "../../../actions.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h" // for extern globals (buttonActions, hash/oled flags)

extern ThrottleManager throttleManager;
extern Renderer renderer;
extern int buttonActions[];
extern bool oledDirectCommandsAreBeingDisplayed;
extern bool hashShowsFunctionsInsteadOfKeyDefs;

void OperationModeHandler::onEnter() {
    // Render speed screen when entering operation mode
    renderer_.renderSpeed();
}

void OperationModeHandler::onExit() {
    // Clean up when leaving operation mode (currently no-op)
}

bool OperationModeHandler::handle(const InputEvent &ev) {
    switch (ev.type) {
        case InputEventType::SpeedDelta: {
            // Apply delta * current speed step through ThrottleManager API.
            int idx = throttleManager.getCurrentThrottleIndex();
            if (!throttle_.hasLocomotive(idx)) return true; // consume but ignore
            int current = throttle_.getCurrentSpeed(idx);
            int step = throttle_.getSpeedStep();
            int next = current + (ev.ivalue * step);
            if (next < 0) next = 0;
            if (next > 127) next = 127; // clamp
            throttle_.speedSet(idx, next);
            #if INPUT_DEBUG
            Serial.print("[OperationModeHandler] SpeedDelta handled; new preview/actual speed: "); Serial.println(next);
            #endif
            return true;
        }
        case InputEventType::SpeedAbsolute: {
            int idx = throttleManager.getCurrentThrottleIndex();
            if (!throttle_.hasLocomotive(idx)) return true; // ignore when no loco
            int val = ev.ivalue;
            if (val < 0) val = 0;
            if (val > 127) val = 127;
            throttle_.speedSet(idx, val);
            return true;
        }
        case InputEventType::EncoderClick: {
            // Implement legacy-configured encoder button behavior.
            int idx = throttleManager.getCurrentThrottleIndex();
            int currentSpeed = throttle_.getCurrentSpeed(idx);
            // ENCODER_BUTTON_ACTION comes from config (default SPEED_STOP_THEN_TOGGLE_DIRECTION)
            if (ENCODER_BUTTON_ACTION == SPEED_STOP_THEN_TOGGLE_DIRECTION) {
                if (currentSpeed > 0) {
                    throttle_.speedSet(idx, 0); // stop first
                } else {
                    throttle_.toggleDirection(idx); // if already stopped, toggle direction
                }
                #if INPUT_DEBUG
                Serial.println("[OperationModeHandler] EncoderClick: stop/toggle applied");
                #endif
                return true;
            } else if (ENCODER_BUTTON_ACTION == DIRECTION_TOGGLE) {
                throttle_.toggleDirection(idx);
                #if INPUT_DEBUG
                Serial.println("[OperationModeHandler] EncoderClick: direction toggled");
                #endif
                return true;
            }
            // If action not recognized, do nothing so other handlers could consume (none now).
            return false;
        }
        case InputEventType::EncoderDoubleClick: {
            // Double-click cycles momentum levels for the CURRENT throttle: Off -> Low -> Med -> High -> Off
            int idx = throttleManager.getCurrentThrottleIndex();
            throttleManager.momentum().cycleMomentumLevel(idx);
            renderer_.renderSpeed(); // Update display to show new momentum indicator
            #if INPUT_DEBUG
            Serial.println("[OperationModeHandler] EncoderDoubleClick: momentum level cycled");
            #endif
            return true;
        }
        case InputEventType::EncoderHold: {
            // Hold activates braking on current locomotive.
            // If the throttle is set above zero, engage service braking: the train
            // decelerates continuously while the encoder is held, with a sound cue
            // appropriate to the loco type.  The displayed speed is unchanged;
            // on release, momentum carries the train back to the set speed.
            // If the throttle is already at zero, engage standard braking to
            // accelerate deceleration to a full stop.
            // These two modes are mutually exclusive — only one fires per hold.
            int idx = throttleManager.getCurrentThrottleIndex();
            if (throttle_.hasLocomotive(idx)) {
                int currentSpeed = throttle_.getCurrentSpeed(idx);
                if (currentSpeed > 0 && throttleManager.momentum().isActive(idx)) {
                    // Service braking only — no standard brake squeal
                    throttleManager.momentum().setServiceBraking(idx, true);
                } else {
                    // Standard braking (throttle at zero or momentum off)
                    throttleManager.momentum().setBraking(idx, true);
                }
                renderer_.renderSpeed();
                #if INPUT_DEBUG
                Serial.println("[OperationModeHandler] EncoderHold: braking activated");
                #endif
            }
            return true;
        }
        case InputEventType::EncoderHoldRelease: {
            // Release deactivates whichever brake mode was engaged
            int idx = throttleManager.getCurrentThrottleIndex();
            if (throttle_.hasLocomotive(idx)) {
                if (throttleManager.momentum().isServiceBraking(idx)) {
                    throttleManager.momentum().setServiceBraking(idx, false);
                } else {
                    throttleManager.momentum().setBraking(idx, false);
                }
                renderer_.renderSpeed();
                #if INPUT_DEBUG
                Serial.println("[OperationModeHandler] EncoderHoldRelease: braking deactivated");
                #endif
            }
            return true;
        }
        case InputEventType::DirectionToggle: {
            int idx = throttleManager.getCurrentThrottleIndex();
            throttle_.toggleDirection(idx);
            return true;
        }
        case InputEventType::EncoderLongPress: {
            // Cycle speed step on long press.
            throttle_.cycleSpeedStep();
            return true;
        }
        case InputEventType::Action: {
            // Map programmable action codes to throttle operations (loco-centric subset only)
            int idx = throttleManager.getCurrentThrottleIndex();
            switch (ev.ivalue) {
                case SPEED_STOP: { if (!throttle_.hasLocomotive(idx)) return true; throttle_.speedSet(idx, 0); return true; }
                case SPEED_UP: { int step = throttle_.getSpeedStep(); if (!throttle_.hasLocomotive(idx)) return true; int next = throttle_.getCurrentSpeed(idx) + step; if (next > 127) next = 127; throttle_.speedSet(idx, next); return true; }
                case SPEED_DOWN: { int step = throttle_.getSpeedStep(); if (!throttle_.hasLocomotive(idx)) return true; int next = throttle_.getCurrentSpeed(idx) - step; if (next < 0) next = 0; throttle_.speedSet(idx, next); return true; }
                case SPEED_UP_FAST: { int step = throttle_.getSpeedStep() * 3; if (!throttle_.hasLocomotive(idx)) return true; int next = throttle_.getCurrentSpeed(idx) + step; if (next > 127) next = 127; throttle_.speedSet(idx, next); return true; }
                case SPEED_DOWN_FAST: { int step = throttle_.getSpeedStep() * 3; if (!throttle_.hasLocomotive(idx)) return true; int next = throttle_.getCurrentSpeed(idx) - step; if (next < 0) next = 0; throttle_.speedSet(idx, next); return true; }
                case SPEED_MULTIPLIER: {
                    throttle_.cycleSpeedStep(); return true; }
                case DIRECTION_TOGGLE: {
                    throttle_.toggleDirection(idx); return true; }
                case DIRECTION_FORWARD: {
                    throttle_.changeDirection(idx, Forward); return true; }
                case DIRECTION_REVERSE: {
                    throttle_.changeDirection(idx, Reverse); return true; }
                case E_STOP_CURRENT_LOCO: 
                { 
                    if (!throttle_.hasLocomotive(idx)) {
                       return true;                     
                    }
                    throttle_.speedSet(idx, 0); 
                    return true; 
                }
                case E_STOP: 
                {
                    throttle_.speedEstopAll();
                    return true; 
                }
                default:
                    return false; // not a loco-centric action we handle here
            }
        }

        // ── Keypad press events ──────────────────────────────────────────
        case InputEventType::KeypadChar:
        case InputEventType::KeypadSpecial: {
            char key = ev.cvalue;

            // If menu is already showing, route to menu system
            if (menu_.isActive()) {
                menu_.handleKey(key);
                return true;
            }

            // '*' activates menu
            if (key == '*') {
                menu_.showMainMenu();
                return true;
            }

            // '#' toggles direct-command display or opens function list
            if (key == '#') {
                if (!hashShowsFunctionsInsteadOfKeyDefs) {
                    if (!oledDirectCommandsAreBeingDisplayed) {
                        renderer_.renderDirectCommands();
                    } else {
                        oledDirectCommandsAreBeingDisplayed = false;
                        renderer_.renderSpeed();
                    }
                } else {
                    if (wiThrottleProtocol.getNumberOfLocomotives(
                            throttleManager.getCurrentThrottleChar()) > 0) {
                        input_.setMode(InputMode::FunctionSelection);
                    }
                }
                return true;
            }

            // Digits 0-9 and A-D execute the configured direct command
            executeDirectCommand(key, true);
            return true;
        }

        // ── Keypad release events ────────────────────────────────────────
        case InputEventType::KeypadCharRelease:
        case InputEventType::KeypadSpecialRelease: {
            char key = ev.cvalue;

            // Ignore releases while menu is active
            if (menu_.isActive()) return true;

            // Only process releases for digits and A-D (momentary functions)
            if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'D')) {
                executeDirectCommand(key, false);
            }
            return true;
        }

        default:
            return false; // not consumed
    }
}

// ── Direct command execution ─────────────────────────────────────────────
void OperationModeHandler::executeDirectCommand(char key, bool pressed) {
    int buttonAction = 0;
    if (key <= '9') {
        buttonAction = buttonActions[key - '0'];
    } else {
        buttonAction = buttonActions[key - 55]; // A=10, B=11, C=12, D=13
    }
    if (buttonAction == FUNCTION_NULL) return;

    if (buttonAction >= FUNCTION_0 && buttonAction <= FUNCTION_31) {
        // Function toggle/momentary — forward press and release
        throttle_.directFunction(throttle_.getCurrentThrottleIndex(), buttonAction, pressed);
    } else if (pressed) {
        // Non-function actions fire on press only
        InputEvent actEv;
        actEv.timestamp = millis();
        actEv.type = InputEventType::Action;
        actEv.ivalue = buttonAction;
        actEv.cvalue = 0;
        input_.dispatch(actEv);    }
}