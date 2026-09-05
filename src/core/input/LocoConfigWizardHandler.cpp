// LocoConfigWizardHandler.cpp — Multi-step loco configuration wizard.
#include "LocoConfigWizardHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../ThrottleManager.h"
#include "../LocoManager.h"
#include "../../../WiTcontroller.h"

extern WiThrottleProtocol wiThrottleProtocol;

LocoConfigWizardHandler::LocoConfigWizardHandler(
    ThrottleManager &throttle, InputManager &input,
    Renderer &renderer, ConfigStore &configStore,
    LocoManager &locoManager)
    : throttle_(throttle), input_(input), renderer_(renderer),
      configStore_(configStore), locoManager_(locoManager) {}

// ═════════════════════════════════════════════════════════════════════════
// Lifecycle
// ═════════════════════════════════════════════════════════════════════════

void LocoConfigWizardHandler::onEnter() {
    int idx = throttle_.getCurrentThrottleIndex();
    char tChar = throttle_.getCurrentThrottleChar();
    locoCount_ = wiThrottleProtocol.getNumberOfLocomotives(tChar);

    // Build list of addresses on this throttle
    for (int i = 0; i < locoCount_ && i < RadioSelectScreen::MAX_OPTIONS; i++) {
        locoAddresses_[i] = wiThrottleProtocol.getLocomotiveAtPosition(tChar, i);
    }

    if (locoCount_ == 0) {
        // No locos — nothing to configure, return to operation
        cancel();
        return;
    }

    if (locoCount_ == 1) {
        // Single loco — skip the pick step, load its config directly
        cfg_ = configStore_.loadLocoConfig(locoAddresses_[0]);
        setupStep(Step::LocoType);
    } else {
        // Consist — ask which loco to configure
        setupStep(Step::PickLoco);
    }
}

void LocoConfigWizardHandler::onExit() {
    radioScreen_.reset();
    textScreen_.clear();
    currentStep_ = Step::PickLoco;
    locoCount_ = 0;
}

// ═════════════════════════════════════════════════════════════════════════
// Event dispatch
// ═════════════════════════════════════════════════════════════════════════

bool LocoConfigWizardHandler::handle(const InputEvent &ev) {
    if (isRadioStep()) {
        return handleRadioInput(ev);
    } else {
        return handleTextInput(ev);
    }
}

bool LocoConfigWizardHandler::isRadioStep() const {
    return currentStep_ == Step::PickLoco ||
           currentStep_ == Step::LocoType;
}

bool LocoConfigWizardHandler::handleRadioInput(const InputEvent &ev) {
    switch (ev.type) {
        case InputEventType::SpeedDelta:
            radioScreen_.moveSelection(ev.ivalue);
            renderer_.renderRadioSelect(radioScreen_);
            return true;

        case InputEventType::EncoderClick:
            advanceFromRadioSelect(radioScreen_.selectedIndex);
            return true;

        case InputEventType::KeypadSpecial:
            if (ev.cvalue == '#') {
                advanceFromRadioSelect(radioScreen_.selectedIndex);
                return true;
            }
            if (ev.cvalue == '*') {
                cancel();
                return true;
            }
            return true;

        case InputEventType::KeypadChar:
            return true; // consume but ignore digit keys

        default:
            return false;
    }
}

bool LocoConfigWizardHandler::handleTextInput(const InputEvent &ev) {
    switch (ev.type) {
        case InputEventType::KeypadChar:
            textScreen_.addChar(ev.cvalue);
            renderer_.renderTextInput(textScreen_);
            return true;

        case InputEventType::KeypadSpecial:
            if (ev.cvalue == '#') {
                advanceFromTextInput();
                return true;
            }
            if (ev.cvalue == '*') {
                if (textScreen_.inputText.length() > 0) {
                    textScreen_.backspace();
                    renderer_.renderTextInput(textScreen_);
                } else {
                    cancel();
                }
                return true;
            }
            return true;

        case InputEventType::EncoderClick:
            advanceFromTextInput();
            return true;

        case InputEventType::SpeedDelta:
            return true; // consume but ignore encoder rotation on text input

        default:
            return false;
    }
}

// ═════════════════════════════════════════════════════════════════════════
// Step setup — configures the screen model for each wizard step
// ═════════════════════════════════════════════════════════════════════════

void LocoConfigWizardHandler::setupStep(Step step) {
    currentStep_ = step;

    if (isRadioStep()) {
        radioScreen_.reset();

        switch (step) {
            case Step::PickLoco:
                radioScreen_.title = "Configure Loco";
                for (int i = 0; i < locoCount_ && i < RadioSelectScreen::MAX_OPTIONS; i++) {
                    radioScreen_.addOption(locoAddresses_[i]);
                }
                break;

            case Step::LocoType:
                radioScreen_.title = "Loco Type";
                radioScreen_.addOption("Diesel");
                radioScreen_.addOption("Steam");
                radioScreen_.addOption("Electric");
                // Pre-select current value
                radioScreen_.selectedIndex = static_cast<int>(cfg_.locoType);
                break;

            default:
                break;
        }

        renderer_.renderRadioSelect(radioScreen_);

    } else {
        // Text input steps for function numbers
        textScreen_.clear();
        textScreen_.footerText = "# OK/Skip  * Del";
        textScreen_.maxLength = 2; // default for F0–F31

        switch (step) {
            case Step::MaxScaleSpeed:
                textScreen_.promptLine1 = cfg_.address;
                textScreen_.promptLine2 = String("Top Speed ") + speedUnitLabel();
                textScreen_.maxLength = 3;
                textScreen_.inputText = String(speedStepToScaleSpeed(cfg_.maxSpeedStep));
                break;

            case Step::FuncThrottleUp:
                textScreen_.promptLine1 = cfg_.address;
                textScreen_.promptLine2 = "Throttle Up F#";
                if (cfg_.funcThrottleUp >= 0)
                    textScreen_.inputText = String(cfg_.funcThrottleUp);
                break;

            case Step::FuncThrottleDown:
                textScreen_.promptLine1 = cfg_.address;
                textScreen_.promptLine2 = "Throttle Down F#";
                if (cfg_.funcThrottleDown >= 0)
                    textScreen_.inputText = String(cfg_.funcThrottleDown);
                break;

            case Step::FuncBrakeSqueal:
                textScreen_.promptLine1 = cfg_.address;
                textScreen_.promptLine2 = "Brake Squeal F#";
                if (cfg_.funcBrakeSqueal >= 0)
                    textScreen_.inputText = String(cfg_.funcBrakeSqueal);
                break;

            case Step::FuncBrakeRelease:
                textScreen_.promptLine1 = cfg_.address;
                textScreen_.promptLine2 = "Brake Release F#";
                if (cfg_.funcBrakeRelease >= 0)
                    textScreen_.inputText = String(cfg_.funcBrakeRelease);
                break;

            case Step::FuncDynamicBrake:
                textScreen_.promptLine1 = cfg_.address;
                textScreen_.promptLine2 = "Dynamic Brake F#";
                if (cfg_.funcDynamicBrake >= 0)
                    textScreen_.inputText = String(cfg_.funcDynamicBrake);
                break;

            case Step::FuncPrimeMoverStart:
                textScreen_.promptLine1 = cfg_.address;
                textScreen_.promptLine2 = "Prime Start F#";
                if (cfg_.funcPrimeMoverStart >= 0)
                    textScreen_.inputText = String(cfg_.funcPrimeMoverStart);
                break;

            case Step::FuncPrimeMoverStop:
                textScreen_.promptLine1 = cfg_.address;
                textScreen_.promptLine2 = "Prime Stop F#";
                if (cfg_.funcPrimeMoverStop >= 0)
                    textScreen_.inputText = String(cfg_.funcPrimeMoverStop);
                break;

            default:
                break;
        }

        renderer_.renderTextInput(textScreen_);
    }
}

// ═════════════════════════════════════════════════════════════════════════
// Step advancement
// ═════════════════════════════════════════════════════════════════════════

void LocoConfigWizardHandler::advanceFromRadioSelect(int selectedIndex) {
    switch (currentStep_) {
        case Step::PickLoco:
            if (selectedIndex >= 0 && selectedIndex < locoCount_) {
                cfg_ = configStore_.loadLocoConfig(locoAddresses_[selectedIndex]);
            }
            setupStep(Step::LocoType);
            break;

        case Step::LocoType:
            cfg_.locoType = static_cast<LocoType>(selectedIndex);
            setupStep(Step::MaxScaleSpeed);
            break;

        default:
            break;
    }
}

void LocoConfigWizardHandler::advanceFromTextInput() {
    // Parse the entered function number (-1 if empty)
    int funcNum = -1;
    if (textScreen_.inputText.length() > 0) {
        funcNum = textScreen_.inputText.toInt();
        if (funcNum < 0 || funcNum > 31) funcNum = -1;
    }

    switch (currentStep_) {
        case Step::MaxScaleSpeed: {
            // Empty input means default full speed.
            int maxScaleSpeed = speedStepToScaleSpeed(126);
            if (textScreen_.inputText.length() > 0) {
                maxScaleSpeed = textScreen_.inputText.toInt();
                if (maxScaleSpeed < 0) maxScaleSpeed = 0;
            }
            cfg_.maxSpeedStep = scaleSpeedToSpeedStep(maxScaleSpeed);

            // Steam locomotives do not use throttle notch sounds.
            if (cfg_.locoType == LocoType::Steam) {
                cfg_.funcThrottleUp = -1;
                cfg_.funcThrottleDown = -1;
                setupStep(Step::FuncBrakeSqueal);
            } else {
                setupStep(Step::FuncThrottleUp);
            }
            break;
        }

        case Step::FuncThrottleUp:
            cfg_.funcThrottleUp = funcNum;
            setupStep(Step::FuncThrottleDown);
            break;

        case Step::FuncThrottleDown:
            cfg_.funcThrottleDown = funcNum;
            setupStep(Step::FuncBrakeSqueal);
            break;

        case Step::FuncBrakeSqueal:
            cfg_.funcBrakeSqueal = funcNum;
            setupStep(Step::FuncBrakeRelease);
            break;

        case Step::FuncBrakeRelease:
            cfg_.funcBrakeRelease = funcNum;
            setupStep(Step::FuncDynamicBrake);
            break;

        case Step::FuncDynamicBrake:
            cfg_.funcDynamicBrake = funcNum;
            setupStep(Step::FuncPrimeMoverStart);
            break;

        case Step::FuncPrimeMoverStart:
            cfg_.funcPrimeMoverStart = funcNum;
            setupStep(Step::FuncPrimeMoverStop);
            break;

        case Step::FuncPrimeMoverStop:
            cfg_.funcPrimeMoverStop = funcNum;
            finish();
            break;

        default:
            break;
    }
}

// ═════════════════════════════════════════════════════════════════════════
// Save & exit
// ═════════════════════════════════════════════════════════════════════════

void LocoConfigWizardHandler::finish() {
    // Derive soundThrottle from whether any function is configured
    cfg_.soundThrottle = (cfg_.funcThrottleUp >= 0 || cfg_.funcThrottleDown >= 0 ||
                          cfg_.funcBrakeSqueal >= 0 || cfg_.funcBrakeRelease >= 0 ||
                          cfg_.funcDynamicBrake >= 0 || cfg_.funcPrimeMoverStart >= 0 ||
                          cfg_.funcPrimeMoverStop >= 0);

    // Persist to flash
    configStore_.saveLocoConfig(cfg_);

    // Live-update momentum controller with new loco type
    int idx = throttle_.getCurrentThrottleIndex();
    throttle_.momentum().setLocoType(idx, cfg_.locoType);

    // Live-update loco caches (sound config + consist speed cap).
    locoManager_.upsertLocoConfigForThrottle(idx, cfg_);

    // Enforce a newly lowered consist speed cap immediately.
    throttle_.speedSet(idx, throttle_.getCurrentSpeed(idx));

    Serial.printf("[LocoConfig] Saved config for %s (type=%d, sound=%d, maxStep=%d)\n",
                  cfg_.address.c_str(), static_cast<int>(cfg_.locoType),
                  cfg_.soundThrottle ? 1 : 0, cfg_.maxSpeedStep);

    input_.setMode(InputMode::Operation);
}

void LocoConfigWizardHandler::cancel() {
    input_.setMode(InputMode::Operation);
}
