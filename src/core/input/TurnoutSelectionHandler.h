#pragma once
#include "PagedListHandler.h"
#include <WiThrottleProtocol.h>

class TurnoutSelectionHandler : public PagedListHandler {
public:
    explicit TurnoutSelectionHandler(Renderer &renderer);

    void setAction(TurnoutAction action) { action_ = action; }
    TurnoutAction getAction() const { return action_; }

protected:
    void configureScreen() override;

private:
    TurnoutAction action_;
};
