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

void WiThrottleServerSelectionHandler::buildDiscoveredScreen() {
    discoveredScreen_ = ListSelectionScreen();
    int count = connectionManager.foundCount();

    discoveredScreen_.totalItems    = count;
    discoveredScreen_.visibleRows   = renderer_.getLayout().ssidItemsPerPage;  // reuse SSID capacity
    discoveredScreen_.halfPageSplit = true;
    discoveredScreen_.footerTemplate = String(menu_text[menu_select_wit_service]);

    discoveredScreen_.itemLabel = [](int gi, bool & /*invert*/) -> String {
        int count = connectionManager.foundCount();
        if (gi >= count) return "";
        const IPAddress *ips   = connectionManager.foundIPs();
        const int       *ports = connectionManager.foundPorts();
        const String    *names = connectionManager.foundNames();
        String ip = ips[gi].toString();
        String truncatedIp = ".." + ip.substring(ip.lastIndexOf("."));
        return truncatedIp + ":" + String(ports[gi]) + " " + names[gi];
    };

    discoveredScreen_.onSelect = [](int index) {
        connectionManager.selectServer(index);
    };

    discoveredScreen_.onBeforeRender = []() {
        menuIsShowing = true;
    };
}

void WiThrottleServerSelectionHandler::onEnter() {
    if (source_ == WiThrottleServerSource::Discovered) {
        buildDiscoveredScreen();
        renderer_.renderListSelection(discoveredScreen_);
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
    if (ev.type != InputEventType::KeypadChar && ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;
    
    if (source_ == WiThrottleServerSource::Discovered) {
        switch (key) {
            case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9': {
                int offset = key - '1';
                int index = offset + (discoveredScreen_.currentPage * discoveredScreen_.visibleRows);
                if (discoveredScreen_.onSelect) discoveredScreen_.onSelect(index);
                return true;
            }

            case '#':
                if (discoveredScreen_.nextPage()) {
                    renderer_.renderListSelection(discoveredScreen_);
                } else {
                    // No more pages — switch to manual entry
                    systemStateManager.setState(SystemState::ServerManualEntry);
                    connectionManager.buildEntry();
                    connectionManager.enterManualServer();
                    setSource(WiThrottleServerSource::ManualEntry);
                }
                return true;
                
            case '*':
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
                    connectionManager.enterManualServer();
                }
                return true;
                
            case '*':
                if (connectionManager.ipAndPortEntered().length() > 0) {
                    connectionManager.ipAndPortEntered() = connectionManager.ipAndPortEntered().substring(0, connectionManager.ipAndPortEntered().length() - 1);
                    connectionManager.ipAndPortChanged() = true;
                    connectionManager.enterManualServer();
                } else {
                    // Cancel — go back to discovered server list
                    setSource(WiThrottleServerSource::Discovered);
                    connectionManager.browseService();
                }
                return true;
                
            case '#':
                if (connectionManager.ipAndPortEntered().length() == 17) {
                    systemStateManager.setState(SystemState::ServerConnecting);
                }
                return true;
                
            default:
                return false;
        }
    }
}
