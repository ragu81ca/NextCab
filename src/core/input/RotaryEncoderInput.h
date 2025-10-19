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
    unsigned long _lastClickMs = 0;  // debounce timestamp for button
};
