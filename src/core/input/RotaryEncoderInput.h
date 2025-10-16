// RotaryEncoderInput.h - wraps AiEsp32RotaryEncoder logic behind IThrottleInput
#pragma once

#include "IThrottleInput.h"

class RotaryEncoderInput : public IThrottleInput {
public:
    RotaryEncoderInput(ThrottleInputEventHandler handler);
    void begin() override;
    void loop() override;
    static void handleISR();

private:
    static RotaryEncoderInput *s_instance; // for ISR callback
    ThrottleInputEventHandler _handler;
    long _lastEncoderValue = 0;      // raw hardware position
    unsigned long _lastClickMs = 0;  // debounce timestamp for button
};
