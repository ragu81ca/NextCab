// ServerConfigWizardHandler.cpp — Per-server settings wizard.
#include "ServerConfigWizardHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../ServerDataStore.h"
#include "../storage/ConfigStore.h"
#include "../network/WiThrottleConnectionManager.h"
#include "../network/ServerSettingsManager.h"
#include "../protocol/WiThrottleDelegate.h"
#include "../../../WiTcontroller.h"

ServerConfigWizardHandler::ServerConfigWizardHandler(
    InputManager &input, Renderer &renderer,
    ServerDataStore &dataStore, ServerSettingsManager &serverSettings)
    : input_(input), renderer_(renderer),
      dataStore_(dataStore), serverSettings_(serverSettings) {}

void ServerConfigWizardHandler::onEnter() {
    turnoutPrefix_ = "";
    setupStep(Step::TurnoutPrefix);
}

void ServerConfigWizardHandler::onExit() {
    screen_.clear();
    currentStep_ = Step::TurnoutPrefix;
}

void ServerConfigWizardHandler::setupStep(Step step) {
    currentStep_ = step;

    screen_.clear();
    screen_.footerText = "E Chrs  E.btn Add  # OK  * Del";
    screen_.maxLength  = 0; // length is managed by the field

    // Seeded from the live value so the screen shows what is actually in effect,
    // including anything filled in by server-type detection.
    switch (step) {
        case Step::TurnoutPrefix:
            screen_.promptLine1 = "Turnout Prefix";
            field_.begin(kCharSet, kCharSetLen, kMaxPrefixLen, dataStore_.turnoutPrefix());
            break;

        case Step::RoutePrefix:
            screen_.promptLine1 = "Route Prefix";
            field_.begin(kCharSet, kCharSetLen, kMaxPrefixLen, dataStore_.routePrefix());
            break;
    }

    render();
}

void ServerConfigWizardHandler::render() {
    field_.applyTo(screen_);
    renderer_.renderTextInput(screen_);
}

bool ServerConfigWizardHandler::handle(const InputEvent &ev) {
    switch (ev.type) {
        case InputEventType::SpeedDelta:
            field_.cycle(ev.ivalue);
            render();
            return true;

        case InputEventType::EncoderClick:
            // With a candidate showing the click confirms it, otherwise it
            // accepts the step.
            if (field_.commitPreview()) {
                render();
            } else {
                advance();
            }
            return true;

        case InputEventType::KeypadChar:
            field_.addChar(ev.cvalue);
            render();
            return true;

        case InputEventType::KeypadSpecial:
            if (ev.cvalue == '#') {
                field_.commitPreview();
                advance();
            } else if (ev.cvalue == '*') {
                if (field_.backspace() == CharEntryField::BackspaceResult::Empty) {
                    cancel();
                } else {
                    render();
                }
            }
            return true;

        case InputEventType::KeypadCharRelease:
        case InputEventType::KeypadSpecialRelease:
            return true;

        default:
            return false;
    }
}

void ServerConfigWizardHandler::advance() {
    switch (currentStep_) {
        case Step::TurnoutPrefix:
            turnoutPrefix_ = field_.value();
            setupStep(Step::RoutePrefix);
            break;

        case Step::RoutePrefix:
            finish();
            break;
    }
}

void ServerConfigWizardHandler::finish() {
    String routePrefix = field_.value();

    serverSettings_.savePrefixes(turnoutPrefix_, routePrefix);

    debug_print("Server config saved: turnout '"); debug_print(turnoutPrefix_);
    debug_print("' route '"); debug_print(routePrefix); debug_println("'");

    cancel();
}

void ServerConfigWizardHandler::cancel() {
    renderer_.renderSpeed();
    input_.setMode(InputMode::Operation);
}
