// PotThrottleInput.h - wraps analog throttle potentiometer logic behind IThrottleInput
#pragma once

#include "IInputDevice.h"
#include "InputEvents.h"

class PotThrottleInput : public IInputDevice {
public:
    explicit PotThrottleInput(IInputDevice::DispatchFn dispatchFn) : _dispatch(dispatchFn) {}
    void begin() override; // configure ADC if needed
    void poll() override;  // read & emit speed events
    const char* name() const override { return "PotThrottle"; }

private:
    IInputDevice::DispatchFn _dispatch { nullptr };
    int _lastSpeed = -1;
    
    // State variables (moved from global scope)
    unsigned long _lastReadTime = 0;
    int _currentNotch = 0;
    int _targetSpeed = 0;
    int _lastValue = 0;
    int _lastHighValue = 0;
    int _smoothingBuffer[5] = {0, 0, 0, 0, 0};
};
