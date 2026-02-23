#pragma once
#include "IModeHandler.h"

// Forward declarations
class Renderer;

class FunctionSelectionHandler : public IModeHandler {
public:
    explicit FunctionSelectionHandler(Renderer &renderer);
    
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;

private:
    Renderer &renderer_;
    int functionPage_;
};
