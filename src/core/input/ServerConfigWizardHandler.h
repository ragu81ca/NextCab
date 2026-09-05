// ServerConfigWizardHandler.h — Per-server settings wizard.
//
// Prefixes are alphanumeric ("LT", "NT", "R") and the keypad has no letters,
// so entry uses the encoder-driven CharEntryField.
#pragma once

#include "IModeHandler.h"
#include "../ui/TextInputScreen.h"
#include "../ui/CharEntryField.h"

class InputManager;
class Renderer;
class ServerDataStore;
class ServerSettingsManager;

class ServerConfigWizardHandler : public IModeHandler {
public:
    ServerConfigWizardHandler(InputManager &input, Renderer &renderer,
                              ServerDataStore &dataStore,
                              ServerSettingsManager &serverSettings);

    void onEnter() override;
    void onExit()  override;
    bool handle(const InputEvent &ev) override;

private:
    enum class Step : uint8_t {
        TurnoutPrefix,
        RoutePrefix
    };

    void setupStep(Step step);
    void advance();
    void finish();
    void cancel();
    void render();

    InputManager          &input_;
    Renderer              &renderer_;
    ServerDataStore       &dataStore_;
    ServerSettingsManager &serverSettings_;

    Step            currentStep_ = Step::TurnoutPrefix;
    TextInputScreen screen_;
    CharEntryField  field_;

    String turnoutPrefix_;

    // Uppercase and digits only — enough for LT/NT/IT/R style names, and short
    // enough to scroll quickly, unlike the password character set.
    static constexpr const char *kCharSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static constexpr size_t kCharSetLen = 36;
    static constexpr size_t kMaxPrefixLen = 5;
};
