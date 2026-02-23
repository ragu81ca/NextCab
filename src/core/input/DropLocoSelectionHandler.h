#pragma once
#include "IModeHandler.h"

// Forward declarations
class Renderer;

class DropLocoSelectionHandler : public IModeHandler {
public:
    explicit DropLocoSelectionHandler(Renderer &renderer);
    
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;

private:
    Renderer &renderer_;
};
