// PotThrottleInput.h - wraps analog throttle potentiometer logic behind IThrottleInput
#pragma once

#include "IThrottleInput.h"
#include "InputEvents.h"

class PotThrottleInput : public IThrottleInput {
public:
    PotThrottleInput(ThrottleInputEventHandler legacyHandler);
    PotThrottleInput(void (*genericHandler)(const InputEvent &)) : _genericHandler(genericHandler) {}
    void begin() override; // configure ADC if needed
    void loop() override;  // read & emit speed events

private:
    ThrottleInputEventHandler _handler { nullptr }; // legacy
    void (*_genericHandler)(const InputEvent &) { nullptr }; // new generic
    int _lastSpeed = -1;
    unsigned long _lastReadMs = 0;
};
