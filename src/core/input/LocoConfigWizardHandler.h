// LocoConfigWizardHandler.h — Multi-step wizard for per-loco configuration.
//
// Steps through RadioSelect and TextInput screens to configure a loco's
// type, sound throttle on/off, and sound function numbers.  Saves to
// ConfigStore and live-updates MomentumController / SoundController state.
#pragma once

#include "IModeHandler.h"
#include "../storage/ConfigStore.h"
#include "../ui/RadioSelectScreen.h"
#include "../ui/TextInputScreen.h"

class ThrottleManager;
class InputManager;
class Renderer;
class LocoManager;

class LocoConfigWizardHandler : public IModeHandler {
public:
    LocoConfigWizardHandler(ThrottleManager &throttle, InputManager &input,
                            Renderer &renderer, ConfigStore &configStore,
                            LocoManager &locoManager);

    void onEnter() override;
    void onExit()  override;
    bool handle(const InputEvent &ev) override;

private:
    // ── Wizard steps ────────────────────────────────────────────────────
    enum class Step : uint8_t {
        PickLoco,       // Only shown when consist has multiple locos
        LocoType,       // Diesel / Steam / Electric
        FuncThrottleUp, // F-number text input (empty = not set)
        FuncThrottleDown,
        FuncBrake,
        FuncDynamicBrake,
        Done
    };

    void setupStep(Step step);
    void advanceFromRadioSelect(int selectedIndex);
    void advanceFromTextInput();
    void finish();
    void cancel();

    // ── Dependencies ────────────────────────────────────────────────────
    ThrottleManager &throttle_;
    InputManager    &input_;
    Renderer        &renderer_;
    ConfigStore     &configStore_;
    LocoManager     &locoManager_;

    // ── State ───────────────────────────────────────────────────────────
    Step              currentStep_ = Step::PickLoco;
    LocoConfig        cfg_;           // Working copy being edited
    RadioSelectScreen radioScreen_;
    TextInputScreen   textScreen_;

    // Loco addresses on current throttle (for consist pick step)
    String locoAddresses_[RadioSelectScreen::MAX_OPTIONS];
    int    locoCount_ = 0;

    // ── Helpers ─────────────────────────────────────────────────────────
    bool handleRadioInput(const InputEvent &ev);
    bool handleTextInput(const InputEvent &ev);
    bool isRadioStep() const;
};
