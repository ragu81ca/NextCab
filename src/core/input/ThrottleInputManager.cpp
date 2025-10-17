// ThrottleInputManager.cpp

#include "ThrottleInputManager.h"
#include <Arduino.h>
#include "WiTcontroller.h"
#include "static.h"               // for ENCODER_USE_* and KEYPAD_USE_* macro constants
#include "src/core/OledRenderer.h" // need full type for render calls
#include "InputManager.h"
#include "InputEvents.h"

// Access current throttle index via ThrottleManager instead of legacy global
#include "src/core/ThrottleManager.h"
extern ThrottleManager throttleManager; // provided by sketch
extern InputManager inputManager; // new generic dispatcher
void speedSet(int throttleIndex, int speed); // existing function
extern bool useRotaryEncoderForThrottle; // existing flag
// We avoid direct dependency on internal speed arrays; we'll query via a lightweight accessor if present.

// Forward declare an accessor if one exists; if not, we'll maintain a cached speed.
int getCurrentThrottleSpeed(int index); // optional (if not defined, linker will discard)
static int cachedSpeed = 0; // fallback cache updated when events processed

// Provide static pointer for callback
static ThrottleInputManager *s_mgr = nullptr;

ThrottleInputManager::ThrottleInputManager() {
    s_mgr = this;
}

void ThrottleInputManager::begin() {
    if (useRotaryEncoderForThrottle) {
        rotary = new RotaryEncoderInput(&ThrottleInputManager::handleEvent);
        impl = rotary;
    } else {
        pot = new PotThrottleInput(&ThrottleInputManager::handleEvent);
        impl = pot;
    }
    if (impl) impl->begin();
}

void ThrottleInputManager::loop() {
    if (impl) impl->loop();
}

void ThrottleInputManager::handleEvent(const ThrottleInputEvent &evt) {
    if (!s_mgr) return;
    switch (evt.type) {
        case ThrottleInputEventType::SpeedDelta: {
            InputEvent gev; gev.type = InputEventType::SpeedDelta; gev.ivalue = evt.value; gev.cvalue = 0; gev.timestamp = millis();
            inputManager.dispatch(gev);
            #if INPUT_DEBUG
            Serial.print("[ThrottleInputManager] SpeedDelta dispatched: "); Serial.println(evt.value);
            #endif
            break;
        }
            case ThrottleInputEventType::SpeedSetAbsolute: {
                InputEvent gev; gev.type = InputEventType::SpeedAbsolute; gev.ivalue = evt.value; gev.cvalue = 0; gev.timestamp = millis();
                inputManager.dispatch(gev);
                break;
            }
        case ThrottleInputEventType::PasswordCharCycle:
            // TODO: integrate password char cycling into abstraction layer.
            break;
        case ThrottleInputEventType::ButtonShortPress: {
            InputEvent gev; gev.type = InputEventType::EncoderClick; gev.ivalue = 1; gev.cvalue = 0; gev.timestamp = millis();
            inputManager.dispatch(gev);
            #if INPUT_DEBUG
            Serial.println("[ThrottleInputManager] EncoderClick dispatched");
            #endif
            break;
        }
    }
}

void ThrottleInputManager::notifySpeedExternallySet(int newSpeed) {
    if (newSpeed < 0) newSpeed = 0; else if (newSpeed > 127) newSpeed = 127;
    cachedSpeed = newSpeed;
}
