#pragma once
#include "PagedListHandler.h"
#include <WiThrottleProtocol.h>

// Sends an explicit Throw/Close derived from last-known state rather than PTA2, so a
// repeat after a dropped DCC packet is harmless instead of inverting the turnout.
void sendTurnoutToggle(const String &sysName);

class TurnoutSelectionHandler : public PagedListHandler {
public:
    explicit TurnoutSelectionHandler(Renderer &renderer);

protected:
    void configureScreen() override;
};
