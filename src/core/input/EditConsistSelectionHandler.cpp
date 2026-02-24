#include "EditConsistSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../ThrottleManager.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern ThrottleManager throttleManager;
extern WiThrottleProtocol wiThrottleProtocol;

EditConsistSelectionHandler::EditConsistSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

int EditConsistSelectionHandler::getItemCount() const {
    return wiThrottleProtocol.getNumberOfLocomotives(
        throttleManager.getCurrentThrottleChar());
}

int EditConsistSelectionHandler::getItemsPerPage() const {
    return renderer_.getLayout().maxLocosDisplayed;
}

String EditConsistSelectionHandler::getItemLabel(int gi, bool &invert) const {
    char tc = throttleManager.getCurrentThrottleChar();
    String loco = wiThrottleProtocol.getLocomotiveAtPosition(tc, gi);
    // Strip S/L prefix, show as suffix only if duplicates exist with different types
    String display = loco;
    if (display.length() > 0 && (display.charAt(0) == 'S' || display.charAt(0) == 'L')) {
        char prefix = display.charAt(0);
        display = display.substring(1);
        // Check for duplicate addresses with different types
        int numLocos = getItemCount();
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
}

String EditConsistSelectionHandler::getFooterText() const {
    return "no Chng Facing   * Close";
}

String EditConsistSelectionHandler::getHeaderText() const {
    return "Edit Consist Facing";
}

void EditConsistSelectionHandler::onBeforeRender() {
    lastOledScreen = last_oled_screen_edit_consist;
    menuIsShowing = false;
}

void EditConsistSelectionHandler::onItemSelected(int index) {
    int numLocos = getItemCount();
    if (index < numLocos && numLocos > 1) {
        String loco = wiThrottleProtocol.getLocomotiveAtPosition(
            throttleManager.getCurrentThrottleChar(), index);
        toggleLocoFacing(throttleManager.getCurrentThrottleIndex(), loco);
    }
    // Always return to operation mode
    inputManager.setMode(InputMode::Operation);
}
