#pragma once
#include <Keypad.h>
#include "IInputDevice.h"
#include "InputEvents.h"

class KeypadInput : public IInputDevice {
public:
    KeypadInput(Keypad &k, IInputDevice::DispatchFn dispatchFn) : _keypad(k), _dispatch(dispatchFn) {}
    void begin() override { /* keypad already configured externally (pins, debounce) */ }
    void poll() override {
        // Use getKeys to obtain state changes (press + release)
        if (_keypad.getKeys()) {
            for (byte i = 0; i < LIST_MAX; i++) {
                if (_keypad.key[i].stateChanged) {
                    char k = _keypad.key[i].kchar;
                    KeyState s = _keypad.key[i].kstate;
                    if (s == PRESSED || s == RELEASED) {
                        InputEvent ev; ev.timestamp = millis(); ev.cvalue = k;
                        bool isSpecial = (k == '*' || k == '#');
                        if (s == PRESSED) {
                            ev.type = isSpecial ? InputEventType::KeypadSpecial : InputEventType::KeypadChar;
                        } else { // RELEASED
                            ev.type = isSpecial ? InputEventType::KeypadSpecialRelease : InputEventType::KeypadCharRelease;
                        }
                        if (_dispatch) _dispatch(ev);
                    }
                }
            }
        }
    }
    const char* name() const override { return "Keypad"; }
private:
    Keypad &_keypad;
    IInputDevice::DispatchFn _dispatch { nullptr };
};
