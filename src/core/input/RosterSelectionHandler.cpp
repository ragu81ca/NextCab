#include "RosterSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../ServerDataStore.h"
#include "../ThrottleManager.h"
#include "../LocoManager.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"
#include "../../core/protocol/WiThrottleDelegate.h"

extern ServerDataStore serverDataStore;
extern InputManager inputManager;
extern ThrottleManager throttleManager;
extern LocoManager locoManager;

RosterSelectionHandler::RosterSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

void RosterSelectionHandler::configureScreen() {
    auto &s = screen();
    s.totalItems    = serverDataStore.rosterSize();
    s.visibleRows   = renderer_.getLayout().rosterItemsPerPage;
    s.footerTemplate = "(%p) " + String(menu_text[menu_roster]);

    s.itemLabel = [this](int gi, bool & /*invert*/) -> String {
        int nameMax = renderer_.getLayout().rosterNameMaxLength;
        int index = serverDataStore.rosterSortedIndex(gi);
        if (serverDataStore.rosterAddress(index) == 0) return "";
        String name = serverDataStore.rosterName(index);
        if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
        return name + " (" + serverDataStore.rosterAddress(index) + ")";
    };

    s.onSelect = [this](int index) {
        if (index >= 0 && index < serverDataStore.rosterSize()) {
            int sortedIdx = serverDataStore.rosterSortedIndex(index);
            String loco = String(serverDataStore.rosterLength(sortedIdx)) + serverDataStore.rosterAddress(sortedIdx);
            debug_print("add Loco: "); debug_println(loco);
            locoManager.acquireLoco(throttleManager.getCurrentThrottleIndex(), loco);
            renderer_.renderSpeed();
            inputManager.setMode(InputMode::Operation);
        }
    };

    s.onBeforeRender = []() {
        lastOledScreen = last_oled_screen_roster;
        lastOledStringParameter = "";
        menuIsShowing = true;
    };
}
