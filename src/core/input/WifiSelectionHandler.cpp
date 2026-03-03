#include "WifiSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../UIState.h"
#include "../network/WifiSsidManager.h"
#include "../ui/TitleScreen.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern UIState uiState;
extern int ssidSelectionSource;

WifiSelectionHandler::WifiSelectionHandler(Renderer &renderer, WifiSsidManager &wifiManager)
    : renderer_(renderer), wifiManager_(wifiManager), source_(WifiSelectionSource::Configured) {}

void WifiSelectionHandler::buildConfiguredScreen() {
    configuredScreen_ = ListSelectionScreen();
    int count = wifiManager_.configuredCount();
    int nameMax = renderer_.getLayout().ssidNameMaxLength;

    configuredScreen_.totalItems    = count;
    configuredScreen_.visibleRows   = renderer_.getLayout().ssidItemsPerPage;
    configuredScreen_.halfPageSplit = true;
    configuredScreen_.footerTemplate = String(menu_text[menu_select_ssids]);

    configuredScreen_.itemLabel = [this, nameMax](int gi, bool & /*invert*/) -> String {
        if (gi >= wifiManager_.configuredCount()) return "";
        String name = wifiManager_.configuredSsid(gi);
        if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax) + "..";
        return name;
    };

    configuredScreen_.onSelect = [this](int index) {
        wifiManager_.selectConfigured(index);
    };

    configuredScreen_.onBeforeRender = []() {
        menuIsShowing = true;
    };
}

void WifiSelectionHandler::buildScannedScreen() {
    scannedScreen_ = ListSelectionScreen();
    int nameMax = renderer_.getLayout().ssidNameMaxLength;

    scannedScreen_.totalItems    = wifiManager_.foundCount();
    scannedScreen_.visibleRows   = renderer_.getLayout().ssidItemsPerPage;
    scannedScreen_.halfPageSplit = false;
    scannedScreen_.footerTemplate = String(menu_text[menu_select_ssids_from_found]);

    scannedScreen_.itemLabel = [this, nameMax](int gi, bool & /*invert*/) -> String {
        if (gi >= wifiManager_.foundCount()) return "";
        String ssid = wifiManager_.foundSsid(gi);
        if (nameMax > 0 && (int)ssid.length() > nameMax) ssid = ssid.substring(0, nameMax);
        // Encode signal strength (0-3) in sentinel char (\x01-\x04)
        int strength = WifiSsidManager::rssiToStrength(wifiManager_.foundRssi(gi));
        return ssid + " " + String((char)('\x01' + strength));
    };

    scannedScreen_.onSelect = [this](int index) {
        wifiManager_.selectFound(index);
    };

    scannedScreen_.onBeforeRender = []() {
        menuIsShowing = true;
    };
}

void WifiSelectionHandler::onEnter() {
    // Sync source from the global selection source so we show the right list
    if (ssidSelectionSource == SSID_CONNECTION_SOURCE_LIST) {
        source_ = WifiSelectionSource::Configured;
    } else {
        source_ = WifiSelectionSource::Scanned;
    }

    if (source_ == WifiSelectionSource::Configured) {
        buildConfiguredScreen();
        renderer_.renderListSelection(configuredScreen_);
    } else {
        if (wifiManager_.foundCount() > 0) {
            buildScannedScreen();
            renderer_.renderListSelection(scannedScreen_);
        } else {
            TitleScreen ts;
            ts.setAppHeader(appName, appVersion);
            ts.addBody(MSG_NO_SSIDS_FOUND);
            ts.footerText = "* Rescan";
            renderer_.renderTitle(ts);
        }
    }
}

void WifiSelectionHandler::onExit() {
    // Nothing to clean up
}

void WifiSelectionHandler::setSource(WifiSelectionSource source) {
    source_ = source;
}

bool WifiSelectionHandler::handle(const InputEvent &ev) {
    if (ev.type != InputEventType::KeypadChar && ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;
    
    if (source_ == WifiSelectionSource::Configured) {
        switch (key) {
            case '1': case '2': case '3': case '4': case '5': 
            case '6': case '7': case '8': case '9': {
                int offset = key - '1';
                int index = offset + (configuredScreen_.currentPage * configuredScreen_.visibleRows);
                if (configuredScreen_.onSelect) configuredScreen_.onSelect(index);
                return true;
            }
                
            case '#': {
                // First try paging, if only 1 page then switch to scanned mode
                if (configuredScreen_.nextPage()) {
                    renderer_.renderListSelection(configuredScreen_);
                } else {
                    setSource(WifiSelectionSource::Scanned);
                    wifiManager_.browseSsids();
                }
                return true;
            }
                
            case '*':
                return true;
                
            default:
                return false;
        }
    } else {
        // Selecting from scanned networks
        switch (key) {
            case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9': {
                int gi = scannedScreen_.globalIndex(key - '1');
                if (gi < scannedScreen_.totalItems) {
                    if (scannedScreen_.onSelect) scannedScreen_.onSelect(gi);
                }
                return true;
            }

            case '#':
                if (scannedScreen_.nextPage()) {
                    renderer_.renderListSelection(scannedScreen_);
                }
                return true;

            case '*':
                // Rescan for available networks
                wifiManager_.browseSsids();
                return true;

            default:
                return false;
        }
    }
}
