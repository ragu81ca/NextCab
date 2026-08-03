#pragma once

#include "IInputDevice.h"
#include "InputEvents.h"

class StemmaRotaryInput : public IInputDevice {
public:
    explicit StemmaRotaryInput(IInputDevice::DispatchFn dispatchFn) : _dispatch(dispatchFn) {}
    // Manual troubleshooting helper; not used in normal startup path.
    static void debugProbeAddresses();
    void begin() override;
    void poll() override;
    const char* name() const override { return "StemmaRotary"; }

private:
    IInputDevice::DispatchFn _dispatch { nullptr };

    bool _ready { false };
    unsigned long _lastPollMs { 0 };
    long _lastRawPosition { 0 };
    int _rawRemainder { 0 };

    int _pendingDelta { 0 };
    unsigned long _pendingDeltaMs { 0 };

    unsigned long _buttonPressStartMs { 0 };
    unsigned long _lastDoubleClickMs { 0 };
    unsigned long _lastButtonEventMs { 0 };

    bool _buttonWasPressed { false };
    bool _longPressTriggered { false };
    bool _waitingForDoubleClick { false };
};
