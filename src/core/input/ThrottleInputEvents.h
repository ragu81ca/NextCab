// ThrottleInputEvents.h - unified events produced by throttle input devices (rotary encoder, potentiometer)
// Part of ongoing refactor to modularize WiTcontroller input handling.

#pragma once

enum class ThrottleInputEventType {
    SpeedSetAbsolute,   // value = absolute target speed 0-127 (pot or future absolute devices)
    SpeedDelta,         // value = signed delta (+/-) to apply to current speed (rotary encoder)
    PasswordCharCycle,  // value = +1 or -1 to move selection (only encoder capable during password entry)
    ButtonShortPress    // momentary press on rotary encoder (if available)
};

struct ThrottleInputEvent {
    ThrottleInputEventType type;
    int value; // interpretation depends on type
};

typedef void (*ThrottleInputEventHandler)(const ThrottleInputEvent &evt);
