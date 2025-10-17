#pragma once
#include <Arduino.h>

// Unified input event types across keypad, encoder, pot, extra buttons, etc.
enum class InputEventType : uint8_t {
    SpeedDelta,        // ivalue = signed delta (clicks)
    SpeedAbsolute,     // ivalue = 0-127 absolute speed
    EncoderClick,      // short press or click
    EncoderLongPress,  // long press action
    KeypadChar,        // cvalue = '0'..'9', letters etc.
    KeypadSpecial,     // cvalue = '*', '#', etc.
    AdditionalButton,  // ivalue = button index
    PasswordCommit,    // signal to finish password entry
    DirectionToggle,   // request to change direction
    SleepRequest,      // request deep sleep (e.g. long inactivity)
    Action             // generic programmable action (ivalue = action code from actions.h)
};

struct InputEvent {
    InputEventType type;
    int ivalue {0};       // generic numeric payload
    char cvalue {0};      // character payload
    unsigned long timestamp {0};
};
