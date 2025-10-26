#pragma once
#include "IModeHandler.h"

// Forward declarations
class OledRenderer;

class FunctionSelectionHandler : public IModeHandler {
public:
    explicit FunctionSelectionHandler(OledRenderer &renderer);
    
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;

private:
    OledRenderer &renderer_;
    int functionPage_;
};
