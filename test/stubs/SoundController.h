// SoundController.h stub for native unit tests.
// Provides the minimal SoundController class interface that MomentumController
// references, without any Arduino/WiThrottle dependencies.
#pragma once

class SoundController {
public:
    // Event handlers called by MomentumController — stubs record calls
    void onBrakeStateChange(int throttle, bool braking) {
        lastBrakeThrottle = throttle;
        lastBrakeState = braking;
        brakeCallCount++;
    }

    void onServiceBrakeStateChange(int throttle, bool active) {
        lastServiceBrakeThrottle = throttle;
        lastServiceBrakeState = active;
        serviceBrakeCallCount++;
    }

    void onSpeedChange(int throttle, int oldSpeed, int newSpeed) {
        lastSpeedThrottle = throttle;
        lastOldSpeed = oldSpeed;
        lastNewSpeed = newSpeed;
        speedChangeCallCount++;
    }

    void onActualSpeedUpdate(int throttle, int actualSpeed) {
        lastActualSpeedThrottle = throttle;
        lastActualSpeed = actualSpeed;
        actualSpeedCallCount++;
    }

    bool isNotching(int /*throttle*/) const { return notching; }
    bool isAnyNotching() const { return notching; }

    // Test inspection
    int lastBrakeThrottle = -1;
    bool lastBrakeState = false;
    int brakeCallCount = 0;

    int lastServiceBrakeThrottle = -1;
    bool lastServiceBrakeState = false;
    int serviceBrakeCallCount = 0;

    int lastSpeedThrottle = -1;
    int lastOldSpeed = -1;
    int lastNewSpeed = -1;
    int speedChangeCallCount = 0;

    int lastActualSpeedThrottle = -1;
    int lastActualSpeed = -1;
    int actualSpeedCallCount = 0;

    // Control notching simulation from tests
    bool notching = false;
};
