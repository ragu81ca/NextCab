#include "WifiSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../UIState.h"
#include "../network/WifiSsidManager.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern UIState uiState;
extern int foundSsidsCount;

WifiSelectionHandler::WifiSelectionHandler(Renderer &renderer, WifiSsidManager &wifiManager)
    : renderer_(renderer), wifiManager_(wifiManager), source_(WifiSelectionSource::Configured), page_(0) {}

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

void WifiSelectionHandler::onEnter() {
    page_ = 0;
    uiState.page = 0;
    
    if (source_ == WifiSelectionSource::Configured) {
        buildConfiguredScreen();
        renderer_.renderListSelection(configuredScreen_);
    } else {
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
    if (ev.type != InputEventType::KeypadChar && ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;
    
    if (source_ == WifiSelectionSource::Configured) {
        switch (key) {
            case '1': case '2': case '3': case '4': case '5': 
            case '6': case '7': case '8': case '9': {
                int offset = (key == '0') ? 9 : (key - '1');
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
                int itemsPerPage = renderer_.getLayout().ssidItemsPerPage;
                int index = (key - '1');
                if (index < itemsPerPage) {
                    wifiManager_.selectFound(index + (page_ * itemsPerPage));
                }
                return true;
            }
                
            case '#': {
                int itemsPerPage = renderer_.getLayout().ssidItemsPerPage;
                if (foundSsidsCount > itemsPerPage) {
                    if ((page_ + 1) * itemsPerPage < foundSsidsCount) {
                        page_++;
                    } else {
                        page_ = 0;
                    }
                    uiState.page = page_;
                    renderer_.renderFoundSsids("");
                }
                return true;
            }
                
            case '0':
                page_ = 0;
                uiState.page = 0;
                wifiManager_.browseSsids();
                return true;
                
            case '*':
                setSource(WifiSelectionSource::Configured);
                return true;
                
            default:
                return false;
        }
    }
}
