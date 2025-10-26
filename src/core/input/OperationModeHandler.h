#pragma once
#include "IModeHandler.h"
#include "../ThrottleManager.h" // adjust relative path

// Handles normal operating throttle events.
class OperationModeHandler : public IModeHandler {
public:
    explicit OperationModeHandler(ThrottleManager &throttle) : throttle_(throttle) {}

    void onEnter() override;
    void onExit() override;
    bool handle(const InputEvent &ev) override;

private:
    ThrottleManager &throttle_;
};
