#pragma once
#include "IModeHandler.h"
#include "../ui/RadioSelectScreen.h"

class DeviceSettingsManager;
class InputManager;
class Renderer;

class DeviceSettingsHandler : public IModeHandler {
public:
    DeviceSettingsHandler(InputManager &input, Renderer &renderer, DeviceSettingsManager &settings);
    void onEnter() override;
    void onExit() override;
    bool handle(const InputEvent &ev) override;

private:
    enum class Step : uint8_t { Heartbeat, ThrottleCount, AcquireMode, RestoreLocos };
    void setupStep(Step step);
    void advance();
    void cancel();

    InputManager &input_;
    Renderer &renderer_;
    DeviceSettingsManager &settings_;
    Step step_ = Step::Heartbeat;
    RadioSelectScreen screen_;
};
