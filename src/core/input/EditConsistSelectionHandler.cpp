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

void EditConsistSelectionHandler::renderCurrentPage() {
    renderer_.renderEditConsist();
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
