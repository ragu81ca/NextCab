#pragma once
#include "IModeHandler.h"
#include "../ui/ListSelectionScreen.h"

// Forward declarations
class Renderer;
class WifiSsidManager;

enum class WifiSelectionSource {
    Configured,  // Selecting from configured networks
    Scanned      // Selecting from scanned networks
};

class WifiSelectionHandler : public IModeHandler {
public:
    explicit WifiSelectionHandler(Renderer &renderer, WifiSsidManager &wifiManager);
    
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;
    
    // Set which source we're selecting from
    void setSource(WifiSelectionSource source);
    WifiSelectionSource getSource() const { return source_; }

private:
    void buildConfiguredScreen();

    Renderer &renderer_;
    WifiSsidManager &wifiManager_;
    WifiSelectionSource source_;
    ListSelectionScreen configuredScreen_;
    int page_;
};
