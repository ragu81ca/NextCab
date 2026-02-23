#include "OperationModeHandler.h"
#include "../Renderer.h"
#include "../ThrottleManager.h"
#include "../../../actions.h" // for SPEED_STOP_THEN_TOGGLE_DIRECTION / DIRECTION_TOGGLE constants
#include "../../../static.h"

extern ThrottleManager throttleManager; // provided by sketch
extern Renderer renderer; // global renderer

void OperationModeHandler::onEnter() {
    // Render speed screen when entering operation mode
    renderer.renderSpeed();
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
            renderer.renderSpeed(); // Update display to show new momentum indicator
            #if INPUT_DEBUG
            Serial.println("[OperationModeHandler] EncoderDoubleClick: momentum level cycled");
            #endif
            return true;
        }
        case InputEventType::EncoderHold: {
            // Hold activates braking on current locomotive
            int idx = throttleManager.getCurrentThrottleIndex();
            if (throttle_.hasLocomotive(idx)) {
                throttleManager.momentum().setBraking(idx, true);
                renderer.renderSpeed(); // Update display to show brake indicator
                #if INPUT_DEBUG
                Serial.println("[OperationModeHandler] EncoderHold: braking activated");
                #endif
            }
            return true;
        }
        case InputEventType::EncoderHoldRelease: {
            // Release deactivates braking on current locomotive
            int idx = throttleManager.getCurrentThrottleIndex();
            if (throttle_.hasLocomotive(idx)) {
                throttleManager.momentum().setBraking(idx, false);
                renderer.renderSpeed(); // Update display to hide brake indicator
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
        default:
            return false; // not consumed
    }
}
