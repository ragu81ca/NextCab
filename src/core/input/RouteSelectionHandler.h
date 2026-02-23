#pragma once
#include "IModeHandler.h"

// Forward declarations
class Renderer;

class RouteSelectionHandler : public IModeHandler {
public:
    explicit RouteSelectionHandler(Renderer &renderer);
    
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;

private:
    Renderer &renderer_;
    int page_;
};
