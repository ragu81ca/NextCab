#pragma once
#include "IModeHandler.h"

// Forward declarations
class OledRenderer;

class EditConsistSelectionHandler : public IModeHandler {
public:
    explicit EditConsistSelectionHandler(OledRenderer &renderer);
    void onEnter() override;
    void onExit() override;
    bool handle(const InputEvent &ev) override;

private:
    OledRenderer &renderer_;
};
