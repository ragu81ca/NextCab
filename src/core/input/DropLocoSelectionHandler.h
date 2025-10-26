#pragma once
#include "IModeHandler.h"

// Forward declarations
class OledRenderer;

class DropLocoSelectionHandler : public IModeHandler {
public:
    explicit DropLocoSelectionHandler(OledRenderer &renderer);
    
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;

private:
    OledRenderer &renderer_;
};
