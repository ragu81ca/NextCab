#include "WifiSelectionHandler.h"
#include "InputManager.h"
#include "../OledRenderer.h"
#include "../UIState.h"
#include "../network/WifiSsidManager.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern UIState uiState;
extern int foundSsidsCount;

WifiSelectionHandler::WifiSelectionHandler(OledRenderer &renderer, WifiSsidManager &wifiManager)
    : renderer_(renderer), wifiManager_(wifiManager), source_(WifiSelectionSource::Configured), page_(0) {}

void WifiSelectionHandler::onEnter() {
    page_ = 0;
    uiState.page = 0;
    
    if (source_ == WifiSelectionSource::Configured) {
        // Show configured networks
        wifiManager_.showConfiguredList();
    } else {
        // Show scanned networks
        renderer_.renderFoundSsids("");
    }
}

void WifiSelectionHandler::onExit() {
    page_ = 0;
}

void WifiSelectionHandler::setSource(WifiSelectionSource source) {
    source_ = source;
    page_ = 0;
    uiState.page = 0;
}

bool WifiSelectionHandler::handle(const InputEvent &ev) {
    // Only handle key press events
    if (ev.type != InputEventType::KeypadChar && ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;
    
    if (source_ == WifiSelectionSource::Configured) {
        // Selecting from configured networks (1-based for users, convert to 0-based indices)
        switch (key) {
            case '1': case '2': case '3': case '4': case '5': 
            case '6': case '7': case '8': case '9':
                wifiManager_.selectConfigured(key - '1');
                // Manager will transition to password entry or WiThrottle selection
                return true;
                
            case '#':  // Switch to scanning mode
                setSource(WifiSelectionSource::Scanned);
                wifiManager_.browseSsids();  // This will trigger scanning and rendering
                return true;
                
            case '*':  // Cancel (not typically used at startup, but could rescan)
                return true;
                
            default:
                return false;
        }
    } else {
        // Selecting from scanned networks (1-based for users, convert to 0-based indices)
        switch (key) {
            case '1': case '2': case '3': case '4': case '5':
                wifiManager_.selectFound((key - '1') + (page_ * 5));
                // Manager will transition to password entry or WiThrottle selection
                return true;
                
            case '#':  // Next page
                if (foundSsidsCount > 5) {
                    if ((page_ + 1) * 5 < foundSsidsCount) {
                        page_++;
                    } else {
                        page_ = 0;
                    }
                    uiState.page = page_;
                    renderer_.renderFoundSsids("");
                }
                return true;
                
            case '9':  // Rescan
                page_ = 0;
                uiState.page = 0;
                wifiManager_.browseSsids();
                return true;
                
            case '*':  // Back to configured networks
                setSource(WifiSelectionSource::Configured);
                // Render configured networks
                return true;
                
            default:
                return false;
        }
    }
}
