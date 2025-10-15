// RotaryEncoderInput.h - wraps AiEsp32RotaryEncoder logic behind IThrottleInput
#pragma once

#include "IThrottleInput.h"

class RotaryEncoderInput : public IThrottleInput {
public:
    RotaryEncoderInput(ThrottleInputEventHandler handler);
    void begin() override;
    void loop() override;

    // Called from ISR to update internal state if needed
    static void handleISR();

private:
    static RotaryEncoderInput *s_instance; // for ISR callback
    ThrottleInputEventHandler _handler;
    long _lastEncoderValue = 0;
    unsigned long _lastButtonMs = 0; // reserved for future long-press/brake logic
    int _absoluteSpeed = 0;          // tracked absolute speed (0-127)
    unsigned long _lastClickMs = 0;  // debounce timestamp
};
