#include "OperationModeHandler.h"
#include "../OledRenderer.h"
#include "../ThrottleManager.h"
#include "../../../actions.h" // for SPEED_STOP_THEN_TOGGLE_DIRECTION / DIRECTION_TOGGLE constants

extern ThrottleManager throttleManager; // provided by sketch
extern OledRenderer oledRenderer; // global renderer

bool OperationModeHandler::handle(const InputEvent &ev) {
    switch (ev.type) {
        case InputEventType::SpeedDelta: {
            // Apply delta * current speed step through ThrottleManager API.
            int idx = throttleManager.getCurrentThrottleIndex();
            int current = throttle_.getCurrentSpeed(idx);
            int step = throttle_.getSpeedStep();
            int next = current + (ev.ivalue * step);
            if (next < 0) next = 0;
            if (next > 127) next = 127; // clamp
            throttle_.speedSet(idx, next);
            return true;
        }
        case InputEventType::SpeedAbsolute: {
            int idx = throttleManager.getCurrentThrottleIndex();
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
                return true;
            } else if (ENCODER_BUTTON_ACTION == DIRECTION_TOGGLE) {
                throttle_.toggleDirection(idx);
                return true;
            }
            // If action not recognized, do nothing so other handlers could consume (none now).
            return false;
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
        default:
            return false; // not consumed
    }
}
