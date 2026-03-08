#include "EditConsistSelectionHandler.h"
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

EditConsistSelectionHandler::EditConsistSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

void EditConsistSelectionHandler::configureScreen() {
    auto &s = screen();
    char tc = throttleManager.getCurrentThrottleChar();
    s.totalItems     = wiThrottleProtocol.getNumberOfLocomotives(tc);
    s.visibleRows    = renderer_.getLayout().maxLocosDisplayed;
    s.halfPageSplit  = true;
    s.headerText     = "Edit Consist Facing";
    s.footerTemplate = "no Chng Facing   * Close";

    s.itemLabel = [this](int gi, bool &invert) -> String {
        char tc = throttleManager.getCurrentThrottleChar();
        String loco = wiThrottleProtocol.getLocomotiveAtPosition(tc, gi);
        // Strip S/L prefix, show as suffix only if duplicates exist with different types
        String display = loco;
        if (display.length() > 0 && (display.charAt(0) == 'S' || display.charAt(0) == 'L')) {
            char prefix = display.charAt(0);
            display = display.substring(1);
            int numLocos = wiThrottleProtocol.getNumberOfLocomotives(tc);
            for (int j = 0; j < numLocos; j++) {
                if (j == gi) continue;
                String other = wiThrottleProtocol.getLocomotiveAtPosition(tc, j);
                if (other.length() > 1 && other.substring(1) == display &&
                    other.charAt(0) != prefix) {
                    display += " (" + String(prefix) + ")";
                    break;
                }
            }
        }
        if (wiThrottleProtocol.getDirection(tc, loco) == Reverse) {
            invert = true;
        }
        return display;
    };

    s.onSelect = [this](int index) {
        int numLocos = wiThrottleProtocol.getNumberOfLocomotives(
            throttleManager.getCurrentThrottleChar());
        if (index < numLocos && numLocos > 1) {
            String loco = wiThrottleProtocol.getLocomotiveAtPosition(
                throttleManager.getCurrentThrottleChar(), index);
            locoManager.toggleLocoFacing(throttleManager.getCurrentThrottleIndex(), loco);
            renderer_.renderSpeed();
            menuCommandStarted = false;
        }
        inputManager.setMode(InputMode::Operation);
    };

    s.onBeforeRender = []() {
        lastOledScreen = last_oled_screen_edit_consist;
        menuIsShowing = false;
    };
}
