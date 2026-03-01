#include "WiThrottleServerSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"
#include "../SystemState.h"
#include "../network/WiThrottleConnectionManager.h"

extern InputManager inputManager;
extern SystemStateManager systemStateManager;
extern WiThrottleConnectionManager connectionManager;

WiThrottleServerSelectionHandler::WiThrottleServerSelectionHandler(Renderer &renderer)
    : renderer_(renderer), source_(WiThrottleServerSource::Discovered) {}

void WiThrottleServerSelectionHandler::onEnter() {
    if (source_ == WiThrottleServerSource::Discovered) {
        // Display already shown by connectionManager.browseService()
    } else {
        // Manual entry mode - display already shown by connectionManager.enterManualServer()
    }
}

void WiThrottleServerSelectionHandler::onExit() {
    // Clean up if needed
}

void WiThrottleServerSelectionHandler::setSource(WiThrottleServerSource source) {
    source_ = source;
}

bool WiThrottleServerSelectionHandler::handle(const InputEvent &ev) {
    // Only handle key press events
    if (ev.type != InputEventType::KeypadChar && ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;
    
    if (source_ == WiThrottleServerSource::Discovered) {
        // Selecting from discovered servers (1-based for users, convert to 0-based indices)
        switch (key) {
            case '1': case '2': case '3': case '4': case '5':
                connectionManager.selectServer(key - '1');
                return true;
                
            case '#':  // Switch to manual entry
                systemStateManager.setState(SystemState::ServerManualEntry);
                connectionManager.buildEntry();
                connectionManager.enterManualServer();
                setSource(WiThrottleServerSource::ManualEntry);
                return true;
                
            case '*':  // Back - rescan or go back to WiFi?
                connectionManager.browseService();
                return true;
                
            default:
                return false;
        }
    } else {
        // Manual IP entry mode
        switch (key) {
            case '0': case '1': case '2': case '3': case '4': 
            case '5': case '6': case '7': case '8': case '9':
                if (connectionManager.ipAndPortEntered().length() < 17) {
                    connectionManager.ipAndPortEntered() += key;
                    connectionManager.ipAndPortChanged() = true;
                    connectionManager.enterManualServer(); // Refresh display
                }
                return true;
                
            case '*':  // Backspace
                if (connectionManager.ipAndPortEntered().length() > 0) {
                    connectionManager.ipAndPortEntered() = connectionManager.ipAndPortEntered().substring(0, connectionManager.ipAndPortEntered().length() - 1);
                    connectionManager.ipAndPortChanged() = true;
                    connectionManager.enterManualServer(); // Refresh display
                }
                return true;
                
            case '#':  // Commit
                if (connectionManager.ipAndPortEntered().length() == 17) {
                    systemStateManager.setState(SystemState::ServerConnecting);
                    // Connection will be attempted in loop()
                }
                return true;
                
            default:
                return false;
        }
    }
}
