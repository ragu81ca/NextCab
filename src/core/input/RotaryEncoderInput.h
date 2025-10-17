// RotaryEncoderInput.h - wraps AiEsp32RotaryEncoder logic behind IThrottleInput
#pragma once

#include "IThrottleInput.h"
#include "InputEvents.h"

class RotaryEncoderInput : public IThrottleInput {
public:
    RotaryEncoderInput(ThrottleInputEventHandler handler); // legacy handler kept until migration complete
    RotaryEncoderInput(void (*genericHandler)(const InputEvent &)) : _genericHandler(genericHandler) {}
    void begin() override;
    void loop() override;
    static void handleISR();

private:
    static RotaryEncoderInput *s_instance; // for ISR callback
    ThrottleInputEventHandler _handler { nullptr }; // legacy
    void (*_genericHandler)(const InputEvent &) { nullptr }; // new generic path
    long _lastEncoderValue = 0;      // raw hardware position
    unsigned long _lastClickMs = 0;  // debounce timestamp for button
};
