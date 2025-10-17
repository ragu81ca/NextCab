#pragma once
#include <Arduino.h>
#include "IModeHandler.h"

// Simple mode id for now; can expand to enum class later.
enum class InputMode : uint8_t {
    Operation,
    PasswordEntry
};

class InputManager {
public:
    InputManager() = default;

    void setOperationHandler(IModeHandler *h) { opHandler_ = h; }
    void setPasswordHandler(IModeHandler *h) { pwHandler_ = h; }

    void setMode(InputMode mode);
    InputMode getMode() const { return mode_; }

    // Dispatch event to active mode; if not consumed can implement fallbacks.
    void dispatch(const InputEvent &ev);

private:
    InputMode mode_ { InputMode::Operation };
    IModeHandler *opHandler_ { nullptr };
    IModeHandler *pwHandler_ { nullptr };
    IModeHandler *active_ { nullptr };
};
