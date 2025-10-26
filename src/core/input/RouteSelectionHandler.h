#pragma once
#include "IModeHandler.h"

// Forward declarations
class OledRenderer;

class RouteSelectionHandler : public IModeHandler {
public:
    explicit RouteSelectionHandler(OledRenderer &renderer);
    
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;

private:
    OledRenderer &renderer_;
    int page_;
};
