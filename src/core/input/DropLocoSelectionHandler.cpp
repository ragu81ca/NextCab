#include "DropLocoSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../ThrottleManager.h"
#include "../LocoManager.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern ThrottleManager throttleManager;
extern WiThrottleProtocol wiThrottleProtocol;
extern LocoManager locoManager;

DropLocoSelectionHandler::DropLocoSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

void DropLocoSelectionHandler::configureScreen() {
    auto &s = screen();
    char tc = throttleManager.getCurrentThrottleChar();
    int numLocos = wiThrottleProtocol.getNumberOfLocomotives(tc);
    bool needSuffixes = renderer_.checkNeedSuffixes(tc, numLocos);

    s.totalItems     = numLocos;
    s.visibleRows    = renderer_.getLayout().maxLocosDisplayed;
    s.halfPageSplit  = true;
    s.headerText     = "Drop Loco";
    s.footerTemplate = "1-9 Select 0 All * Cancel";

    s.itemLabel = [this, needSuffixes](int gi, bool &invert) -> String {
        char tc = throttleManager.getCurrentThrottleChar();
        String loco = wiThrottleProtocol.getLocomotiveAtPosition(tc, gi);
        if (wiThrottleProtocol.getDirection(tc, loco) == Reverse) {
            invert = true;
        }
        return renderer_.formatLocoDisplay(loco, needSuffixes);
    };

    s.onSelect = [](int index) {
        char tc = throttleManager.getCurrentThrottleChar();
        int numLocos = wiThrottleProtocol.getNumberOfLocomotives(tc);
        if (index < numLocos) {
            locoManager.releaseOneLocoByIndex(throttleManager.getCurrentThrottleIndex(), index);
        }
        extern InputManager inputManager;
        inputManager.setMode(InputMode::Operation);
    };

    s.onBeforeRender = []() {
        lastOledScreen = last_oled_screen_all_locos;
        menuIsShowing = true;
    };
}

bool DropLocoSelectionHandler::handleExtraKey(char key) {
    if (key == '0') {
        locoManager.releaseAllLocos(throttleManager.getCurrentThrottleIndex());
        inputManager.setMode(InputMode::Operation);
        return true;
    }
    return false;
}
