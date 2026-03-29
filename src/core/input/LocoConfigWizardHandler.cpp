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
        textScreen_.maxLength = 2; // F0–F31

        switch (step) {
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

            case Step::FuncBrake:
                textScreen_.promptLine1 = cfg_.address;
                textScreen_.promptLine2 = "Brake Sound F#";
                if (cfg_.funcBrake >= 0)
                    textScreen_.inputText = String(cfg_.funcBrake);
                break;

            case Step::FuncDynamicBrake:
                textScreen_.promptLine1 = cfg_.address;
                textScreen_.promptLine2 = "Dynamic Brake F#";
                if (cfg_.funcDynamicBrake >= 0)
                    textScreen_.inputText = String(cfg_.funcDynamicBrake);
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
            setupStep(Step::FuncThrottleUp);
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
        case Step::FuncThrottleUp:
            cfg_.funcThrottleUp = funcNum;
            setupStep(Step::FuncThrottleDown);
            break;

        case Step::FuncThrottleDown:
            cfg_.funcThrottleDown = funcNum;
            setupStep(Step::FuncBrake);
            break;

        case Step::FuncBrake:
            cfg_.funcBrake = funcNum;
            setupStep(Step::FuncDynamicBrake);
            break;

        case Step::FuncDynamicBrake:
            cfg_.funcDynamicBrake = funcNum;
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
                          cfg_.funcBrake >= 0 || cfg_.funcDynamicBrake >= 0);

    // Persist to flash
    configStore_.saveLocoConfig(cfg_);

    // Live-update momentum controller with new loco type
    int idx = throttle_.getCurrentThrottleIndex();
    throttle_.momentum().setLocoType(idx, cfg_.locoType);

    // Live-update sound config cache in LocoManager:
    // Remove old entry for this address and re-add if sound enabled
    auto &vec = const_cast<std::vector<LocoConfig>&>(locoManager_.soundConfigs(idx));
    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
            [&](const LocoConfig &c) { return c.address == cfg_.address; }),
        vec.end());
    if (cfg_.soundThrottle) {
        vec.push_back(cfg_);
    }

    Serial.printf("[LocoConfig] Saved config for %s (type=%d, sound=%d)\n",
                  cfg_.address.c_str(), static_cast<int>(cfg_.locoType),
                  cfg_.soundThrottle ? 1 : 0);

    input_.setMode(InputMode::Operation);
}

void LocoConfigWizardHandler::cancel() {
    input_.setMode(InputMode::Operation);
}
