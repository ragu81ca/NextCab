// ThrottleManager.h - extracted throttle logic (phase 1)
#pragma once

#include <Arduino.h>
#include "static.h"
#include "WiThrottleProtocol.h"

class ThrottleManager {
public:
    ThrottleManager();
    void begin(WiThrottleProtocol *proto);

    void speedUp(int throttle, int amt);
    void speedDown(int throttle, int amt);
    void speedSet(int throttle, int value);
    int  getDisplaySpeed(int throttle) const;
    void speedEstopAll();
    void speedEstopCurrent();

    void changeDirection(int throttle, Direction dir);
    void toggleDirection(int throttle);

    void nextThrottle();
    void selectThrottle(int idx);
    void changeNumberOfThrottles(bool increase);
    void toggleAdditionalMultiplier();

private:
    WiThrottleProtocol *proto { nullptr };
    void writeSpeedIfVisible(int throttle);
};
