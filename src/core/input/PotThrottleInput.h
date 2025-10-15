// PotThrottleInput.h - wraps analog throttle potentiometer logic behind IThrottleInput
#pragma once

#include "IThrottleInput.h"

class PotThrottleInput : public IThrottleInput {
public:
    PotThrottleInput(ThrottleInputEventHandler handler);
    void begin() override; // configure ADC if needed
    void loop() override;  // read & emit speed events

private:
    ThrottleInputEventHandler _handler;
    int _lastSpeed = -1;
    unsigned long _lastReadMs = 0;
};
