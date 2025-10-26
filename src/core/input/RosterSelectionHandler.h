#pragma once
#include "IModeHandler.h"

// Forward declarations
class OledRenderer;

class RosterSelectionHandler : public IModeHandler {
public:
    explicit RosterSelectionHandler(OledRenderer &renderer);
    
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;

private:
    OledRenderer &renderer_;
    int page_;
};
