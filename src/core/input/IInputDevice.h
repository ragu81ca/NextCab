#pragma once
#include <functional>
#include "InputEvents.h"

// Minimal common interface for any polled input source that produces InputEvents.
class IInputDevice {
public:
    using DispatchFn = std::function<void(const InputEvent&)>;
    virtual ~IInputDevice() = default;
    virtual void begin() = 0;          // hardware / internal init
    virtual void poll() = 0;           // called each main loop iteration
    virtual const char* name() const = 0; // for diagnostics / logging
};
