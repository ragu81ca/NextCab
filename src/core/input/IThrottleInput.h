// IThrottleInput.h - interface for any throttle input source.

#pragma once

#include "ThrottleInputEvents.h"

class IThrottleInput {
public:
    virtual ~IThrottleInput() = default;
    virtual void begin() = 0; // hardware setup
    virtual void loop() = 0;  // poll / debounce; emit events via handler
};
