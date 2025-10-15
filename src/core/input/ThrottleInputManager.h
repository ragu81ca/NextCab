// ThrottleInputManager.h - orchestrates chosen throttle input device
#pragma once

#include "IThrottleInput.h"
#include "RotaryEncoderInput.h"
#include "PotThrottleInput.h"

class ThrottleInputManager {
public:
    ThrottleInputManager();
    void begin();
    void loop();

private:
    static void handleEvent(const ThrottleInputEvent &evt);
    IThrottleInput *impl = nullptr; // non-owning; points to one of the concrete members
    RotaryEncoderInput *rotary = nullptr;
    PotThrottleInput *pot = nullptr;
};
