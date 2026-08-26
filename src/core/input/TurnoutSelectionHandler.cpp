#include "TurnoutSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../ServerDataStore.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"
#include "../../core/protocol/WiThrottleDelegate.h"

extern ServerDataStore serverDataStore;
extern InputManager inputManager;
extern WiThrottleProtocol wiThrottleProtocol;

void sendTurnoutToggle(const String &sysName) {
    int idx   = serverDataStore.turnoutIndexBySysName(sysName);
    int state = (idx < 0) ? TurnoutUnknown : serverDataStore.turnoutState(idx);
    TurnoutAction action = (state == TurnoutThrown) ? TurnoutClose : TurnoutThrow;
    debug_print("Turnout toggle: '"); debug_print(sysName);
    debug_print("' listIndex "); debug_print(idx);
    debug_print(" state "); debug_print(state);
    debug_print(" -> action "); debug_println(action);
    wiThrottleProtocol.setTurnout(sysName, action);
}

TurnoutSelectionHandler::TurnoutSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

void TurnoutSelectionHandler::configureScreen() {
    auto &s = screen();
    s.totalItems     = serverDataStore.turnoutListSize();
    s.visibleRows    = renderer_.getLayout().turnoutItemsPerPage;
    s.halfPageSplit  = true;
    s.footerTemplate = "(%p) " + String(menu_text[menu_turnout_list]);

    s.itemLabel = [this](int gi, bool & /*invert*/) -> String {
        int nameMax = renderer_.getLayout().turnoutNameMaxLength;
        // Fall back to the system name, otherwise unnamed turnouts render as a blank
        // row that is still selectable by its position.
        String name = serverDataStore.turnoutUserName(gi);
        if (name.length() == 0) name = serverDataStore.turnoutSysName(gi);
        if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
        return name;
    };

    s.onSelect = [this](int index) {
        if (index >= 0 && index < serverDataStore.turnoutListSize()) {
            sendTurnoutToggle(serverDataStore.turnoutSysName(index));
            renderer_.renderSpeed();
            inputManager.setMode(InputMode::Operation);
        }
    };

    s.onBeforeRender = []() {
        lastOledScreen = last_oled_screen_turnout_list;
        lastOledStringParameter = "";
        menuIsShowing = true;
    };
}
