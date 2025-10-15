// ThrottleInputManager.cpp

#include "ThrottleInputManager.h"
#include <Arduino.h>
#include "WiTcontroller.h"
#include "static.h"               // for ENCODER_USE_* and KEYPAD_USE_* macro constants
#include "src/core/OledRenderer.h" // need full type for render calls

// Forward declarations for speed and password handling from sketch context
extern int currentThrottleIndex; // defined in sketch
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
            case ThrottleInputEventType::SpeedSetAbsolute: {
                int newSpeed = evt.value;
                if (newSpeed < 0) newSpeed = 0; else if (newSpeed > 127) newSpeed = 127;
                speedSet(currentThrottleIndex, newSpeed);
                cachedSpeed = newSpeed;
                break;
            }
        case ThrottleInputEventType::PasswordCharCycle:
            // TODO: integrate password char cycling into abstraction layer.
            break;
        case ThrottleInputEventType::ButtonShortPress: {
            // Inline former rotary_onButtonClick() logic (debounce already handled in input)
            extern int encoderUseType;
            extern int keypadUseType;
            extern int encoderButtonAction;
            // constants are macros (#defines) from static.h included via WiTcontroller.h
            extern char ssidPasswordCurrentChar;
            extern const char ssidPasswordBlankChar; // defined in static.h
            extern String ssidPasswordEntered;
            extern bool ssidPasswordChanged;
            void deepSleepStart();
            void doDirectAction(int action);
            void speedSet(int throttleIndex, int speed);
            void toggleDirection(int throttleIndex);
            void refreshOled();
            // renderer already included at top

            if (encoderUseType == ENCODER_USE_OPERATION) {
                if ( (keypadUseType!=KEYPAD_USE_SELECT_WITHROTTLE_SERVER)
                    && (keypadUseType!=KEYPAD_USE_ENTER_WITHROTTLE_SERVER)
                    && (keypadUseType!=KEYPAD_USE_SELECT_SSID)
                    && (keypadUseType!=KEYPAD_USE_SELECT_SSID_FROM_FOUND) ) {
                    // Encoder used for normal operation
                    doDirectAction(encoderButtonAction);
                    oledRenderer.renderSpeed();
                } else {
                    deepSleepStart();
                }
            } else if (encoderUseType == ENCODER_USE_SSID_PASSWORD) {
                if (ssidPasswordCurrentChar!=ssidPasswordBlankChar) {
                    ssidPasswordEntered += ssidPasswordCurrentChar;
                    ssidPasswordCurrentChar = ssidPasswordBlankChar;
                    oledRenderer.renderEnterPassword();
                    ssidPasswordChanged = true; // ensure UI updates if needed
                }
            }
            break;
        }
    }
}
