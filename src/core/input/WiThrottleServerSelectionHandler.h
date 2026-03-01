#pragma once
#include "IModeHandler.h"
#include "../ui/ListSelectionScreen.h"

// Forward declarations
class Renderer;
class WiThrottleConnectionManager;

enum class WiThrottleServerSource {
    Discovered,  // Selecting from discovered servers
    ManualEntry  // Manually entering IP address
};

class WiThrottleServerSelectionHandler : public IModeHandler {
public:
    explicit WiThrottleServerSelectionHandler(Renderer &renderer);
    
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;
    
    void setSource(WiThrottleServerSource source);
    WiThrottleServerSource getSource() const { return source_; }

private:
    void buildDiscoveredScreen();

    Renderer &renderer_;
    WiThrottleServerSource source_;
    ListSelectionScreen discoveredScreen_;
};
