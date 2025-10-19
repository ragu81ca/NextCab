#pragma once
#include <Arduino.h>
#include <functional>
#include "InputEvents.h"
#include "../../../actions.h" // for FUNCTION_0..FUNCTION_31 range constants

struct AdditionalButtonDef {
    uint8_t pin;              // GPIO pin
    uint8_t mode;             // INPUT or INPUT_PULLUP
    int actionCode;           // existing action code (maps to DomainAction or function)
    bool toggle;              // treat as toggle (dispatch press only; release ignored)
    bool oneShot;             // fire only on first press until released
};

class AdditionalButtonsInput {
public:
    using DispatchFn = std::function<void(const InputEvent&)>;
    AdditionalButtonsInput(DispatchFn fn) : dispatch(fn) {}

    void begin(const AdditionalButtonDef* defs, size_t count, unsigned long debounceMs=40) {
        definitions = defs; defCount = count; debounceDelay = debounceMs;
        lastState = new uint8_t[count];
        lastStableState = new uint8_t[count];
        lastChangeTime = new unsigned long[count];
        pulseEmitted = new bool[count];
        for (size_t i=0;i<count;i++) {
            pinMode(defs[i].pin, defs[i].mode);
            uint8_t read = digitalRead(defs[i].pin);
            lastState[i] = read; lastStableState[i] = read; lastChangeTime[i] = 0;
            pulseEmitted[i] = false;
            // Optional: initial invert for pullups so logical pressed becomes LOW
            // init instrumentation removed for production
        }
    }

    void loop() {
        unsigned long now = millis();
        for (size_t i=0;i<defCount;i++) {
            uint8_t raw = digitalRead(definitions[i].pin);
            if (raw != lastState[i]) { // potential edge
                lastChangeTime[i] = now;
                lastState[i] = raw;
                // removed per-production raw change logging
                // Immediate edge handling for very short pulses (e.g. E_STOP) that may bounce back before debounce period.
                bool pressedEdge = isPressed(i, raw);
                const auto &def = definitions[i];
                bool isFunction = (def.actionCode >= FUNCTION_0 && def.actionCode <= FUNCTION_31);
                // Immediate edge only for emergency stop actions; others wait for debounce to avoid double triggers.
                if (pressedEdge && !isFunction && !pulseEmitted[i] && (def.actionCode == E_STOP || def.actionCode == E_STOP_CURRENT_LOCO)) {
                    // Fire immediately; mark so we don't duplicate on stable phase.
                    emitEvent(i, true, now);
                    pulseEmitted[i] = true;
                }
                // For function buttons (including latching), emit press immediately and mark pulseEmitted to suppress duplicate press during debounce.
                if (pressedEdge && isFunction && !pulseEmitted[i]) {
                    emitEvent(i, true, now);
                    pulseEmitted[i] = true; // hold until debounced release so only one press event occurs
                }
                // Only clear pulseEmitted early for non-function emergency pulses; function buttons clear after debounce release handling.
                if (!pressedEdge && pulseEmitted[i] && !isFunction) {
                    pulseEmitted[i] = false;
                }
            }
            // Debounce: confirm stable change
            if ((now - lastChangeTime[i]) >= debounceDelay && lastStableState[i] != lastState[i]) {
                bool pressed = isPressed(i, lastState[i]);
                if (pulseEmitted[i] && !pressed) {
                    // We already emitted press immediately; optionally emit release for non-toggle, non-oneShot.
                    const auto &def = definitions[i];
                    if (!def.toggle && !def.oneShot) {
                        emitEvent(i, false, now);
                    }
                    pulseEmitted[i] = false; // clear
                    lastStableState[i] = lastState[i];
                    continue; // handled
                }
                if (pulseEmitted[i] && pressed) {
                    // Already emitted press; skip duplicate.
                    lastStableState[i] = lastState[i];
                    continue;
                }
                handleStateChange(i, pressed, now);
                lastStableState[i] = lastState[i];
            }
        }
    }

private:
    DispatchFn dispatch;
    const AdditionalButtonDef* definitions { nullptr };
    size_t defCount { 0 };
    unsigned long debounceDelay { 40 };
    uint8_t* lastState { nullptr };            // instantaneous reading
    uint8_t* lastStableState { nullptr };      // debounced state
    unsigned long* lastChangeTime { nullptr }; // time of last instantaneous change
    bool* pulseEmitted { nullptr };             // tracks immediate emission for critical short pulses

    bool isPressed(size_t idx, uint8_t raw) const {
        // For INPUT_PULLUP pressed is LOW; for INPUT pressed is HIGH
        return (definitions[idx].mode == INPUT_PULLUP) ? (raw == LOW) : (raw == HIGH);
    }

    void handleStateChange(size_t idx, bool pressed, unsigned long now) {
        const auto &def = definitions[idx];
        if (pressed) {
            if (def.oneShot) {
                // Only emit on press; ignore releases until next press
                emitEvent(idx, true, now);
            } else if (def.toggle) {
                // Emit press only; toggling handled downstream
                emitEvent(idx, true, now);
            } else {
                emitEvent(idx, true, now);
            }
        } else {
            if (!def.toggle && !def.oneShot) {
                emitEvent(idx, false, now);
            }
        }
    }

    void emitEvent(size_t idx, bool pressed, unsigned long now) {
        const auto &def = definitions[idx];
        int actionCode = def.actionCode;
        if (actionCode == FUNCTION_NULL) return;
        InputEvent ev; ev.timestamp = now;
        bool isFunction = (actionCode >= FUNCTION_0 && actionCode <= FUNCTION_31);
        if (isFunction) {
            ev.type = InputEventType::AdditionalButton; ev.ivalue = (int)idx; ev.cvalue = pressed ? 'P' : 'R';
        } else {
            if (!pressed) return; // non-function actions fire on press only (debounced)
            ev.type = InputEventType::Action; ev.ivalue = actionCode; ev.cvalue = 0;
        }
        dispatch(ev);
    }
};
