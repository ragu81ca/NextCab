// RotaryEncoderInput.h - wraps AiEsp32RotaryEncoder logic behind IThrottleInput
#pragma once

#include "IInputDevice.h"
#include "InputEvents.h"

class RotaryEncoderInput : public IInputDevice {
public:
    explicit RotaryEncoderInput(IInputDevice::DispatchFn dispatchFn) : _dispatch(dispatchFn) {}
    void begin() override;
    void poll() override; // replaces loop
    const char* name() const override { return "RotaryEncoder"; }
    static void handleISR();

private:
    static RotaryEncoderInput *s_instance; // for ISR callback
    IInputDevice::DispatchFn _dispatch { nullptr };
    long _lastEncoderValue = 0;      // raw hardware position
    int  _pendingDelta = 0;          // buffered rotation awaiting dispatch
    unsigned long _pendingDeltaMs = 0; // when the pending delta was first recorded
    unsigned long _lastClickMs = 0;  // debounce timestamp for button
    unsigned long _buttonPressStartMs = 0; // track when button was pressed
    unsigned long _lastDoubleClickMs = 0;   // for double-click detection
    unsigned long _lastButtonEventMs = 0;   // track last button press/release for rotation filtering
    bool _buttonWasPressed = false;  // track button state
    bool _longPressTriggered = false; // prevent multiple long-press events
    bool _waitingForDoubleClick = false; // true when waiting for potential second click
};
