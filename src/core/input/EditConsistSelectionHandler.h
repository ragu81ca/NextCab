#pragma once
#include "IModeHandler.h"

// Forward declarations
class Renderer;

class EditConsistSelectionHandler : public IModeHandler {
public:
    explicit EditConsistSelectionHandler(Renderer &renderer);
    void onEnter() override;
    void onExit() override;
    bool handle(const InputEvent &ev) override;

private:
    Renderer &renderer_;
};
