#pragma once
#include "InputEvents.h"

// Base interface for a mode handler. Each mode interprets input events differently.
class IModeHandler {
public:
    virtual ~IModeHandler() = default;

    // Return true if the event was consumed by the mode.
    virtual bool handle(const InputEvent &ev) = 0;

    // Called when the mode becomes active.
    virtual void onEnter() {}

    // Called when the mode is about to be deactivated.
    virtual void onExit() {}
};
