#pragma once
#include "IModeHandler.h"
#include "../ThrottleManager.h" // adjust relative path

class MenuSystem;   // forward declare
class InputManager; // forward declare
class Renderer;     // forward declare

// Handles normal operating throttle events, keypad routing, and direct commands.
class OperationModeHandler : public IModeHandler {
public:
    OperationModeHandler(ThrottleManager &throttle, MenuSystem &menu,
                         InputManager &input, Renderer &renderer)
        : throttle_(throttle), menu_(menu), input_(input), renderer_(renderer) {}

    void onEnter() override;
    void onExit() override;
    bool handle(const InputEvent &ev) override;

private:
    ThrottleManager &throttle_;
    MenuSystem      &menu_;
    InputManager    &input_;
    Renderer        &renderer_;

    /// Map a keypad character to its buttonActions[] index and execute the
    /// configured action (function toggle or programmable action).
    void executeDirectCommand(char key, bool pressed);
};
