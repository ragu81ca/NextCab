#pragma once
#include <Arduino.h>

// Unified input event types across keypad, encoder, pot, extra buttons, etc.
enum class InputEventType : uint8_t {
    SpeedDelta,           // ivalue = signed delta (clicks)
    SpeedAbsolute,        // ivalue = 0-127 absolute speed
    EncoderClick,         // short press or click
    EncoderLongPress,     // long press action
    // Keypad press events
    KeypadChar,           // key press: cvalue = '0'..'9', letters etc.
    KeypadSpecial,        // key press: cvalue = '*', '#', etc.
    // Keypad release events (needed for momentary functions / UI feedback)
    KeypadCharRelease,    // key release: cvalue = original key
    KeypadSpecialRelease, // key release: cvalue = original special key
    AdditionalButton,     // ivalue = button index (cvalue='P'/'R' for press/release)
    PasswordCommit,       // signal to finish password entry
    DirectionToggle,      // request to change direction
    SleepRequest,         // request deep sleep (e.g. long inactivity)
    Action                // generic programmable action (ivalue = action code from actions.h)
};

struct InputEvent {
    InputEventType type;
    int ivalue {0};       // generic numeric payload
    char cvalue {0};      // character payload
    unsigned long timestamp {0};
};
