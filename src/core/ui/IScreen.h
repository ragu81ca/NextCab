// IScreen.h — Base interface for all screen types.
// Screens are data-only: they hold content and callbacks but know NOTHING
// about pixels, fonts, or layout.  The Renderer owns all spatial decisions.
#pragma once
#include <Arduino.h>

enum class ScreenType : uint8_t {
    Operating,
    Title,
    Wait,
    ListSelection,
    RadioSelect,
    TextInput,
    Confirmation
};

class IScreen {
public:
    virtual ~IScreen() = default;
    virtual ScreenType type() const = 0;
};
