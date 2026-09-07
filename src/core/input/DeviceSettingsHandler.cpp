#include "DeviceSettingsHandler.h"
#include "InputManager.h"
#include "../DeviceSettingsManager.h"
#include "../Renderer.h"

DeviceSettingsHandler::DeviceSettingsHandler(InputManager &input, Renderer &renderer,
                                             DeviceSettingsManager &settings)
    : input_(input), renderer_(renderer), settings_(settings) {}

void DeviceSettingsHandler::onEnter() { setupStep(Step::Heartbeat); }
void DeviceSettingsHandler::onExit() { screen_.reset(); }

void DeviceSettingsHandler::setupStep(Step step) {
    step_ = step;
    screen_.reset();
    switch (step_) {
        case Step::Heartbeat:
            screen_.title = "Heartbeat";
            screen_.addOption("On"); screen_.addOption("Off");
            screen_.selectedIndex = settings_.heartbeatEnabled() ? 0 : 1;
            break;
        case Step::ThrottleCount:
            screen_.title = "# Throttles";
            for (int count = 1; count <= WIT_MAX_THROTTLES; count++) screen_.addOption(String(count));
            screen_.selectedIndex = settings_.numberOfThrottles() - 1;
            break;
        case Step::AcquireMode:
            screen_.title = "Acquire Mode";
            screen_.addOption("Replace"); screen_.addOption("Consist");
            screen_.selectedIndex = settings_.dropBeforeAcquire() ? 0 : 1;
            break;
        case Step::RestoreLocos:
            screen_.title = "Remember Locos";
            screen_.addOption("On"); screen_.addOption("Off");
            screen_.selectedIndex = settings_.restoreAcquiredLocos() ? 0 : 1;
            break;
    }
    renderer_.renderRadioSelect(screen_);
}

bool DeviceSettingsHandler::handle(const InputEvent &ev) {
    if (ev.type == InputEventType::SpeedDelta) {
        screen_.moveSelection(ev.ivalue); renderer_.renderRadioSelect(screen_); return true;
    }
    if (ev.type == InputEventType::EncoderClick || (ev.type == InputEventType::KeypadSpecial && ev.cvalue == '#')) {
        advance(); return true;
    }
    if (ev.type == InputEventType::KeypadSpecial && ev.cvalue == '*') { cancel(); return true; }
    return ev.type == InputEventType::KeypadChar;
}

void DeviceSettingsHandler::advance() {
    switch (step_) {
        case Step::Heartbeat: settings_.setHeartbeatEnabled(screen_.selectedIndex == 0); setupStep(Step::ThrottleCount); break;
        case Step::ThrottleCount: settings_.setNumberOfThrottles(screen_.selectedIndex + 1); setupStep(Step::AcquireMode); break;
        case Step::AcquireMode: settings_.setDropBeforeAcquire(screen_.selectedIndex == 0); setupStep(Step::RestoreLocos); break;
        case Step::RestoreLocos: settings_.setRestoreAcquiredLocos(screen_.selectedIndex == 0); cancel(); break;
    }
}

void DeviceSettingsHandler::cancel() { input_.setMode(InputMode::Operation); }
