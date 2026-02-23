#pragma once
#include "IModeHandler.h"
#include <WiThrottleProtocol.h>

// Forward declarations
class Renderer;

class TurnoutSelectionHandler : public IModeHandler {
public:
    explicit TurnoutSelectionHandler(Renderer &renderer);
    
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;
    
    void setAction(TurnoutAction action) { action_ = action; }
    TurnoutAction getAction() const { return action_; }

private:
    Renderer &renderer_;
    int page_;
    TurnoutAction action_;
};
