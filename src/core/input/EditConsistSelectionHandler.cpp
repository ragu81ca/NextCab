#include "EditConsistSelectionHandler.h"
#include "InputManager.h"
#include "../OledRenderer.h"
#include "../ThrottleManager.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern ThrottleManager throttleManager;
extern WiThrottleProtocol wiThrottleProtocol;

EditConsistSelectionHandler::EditConsistSelectionHandler(OledRenderer &renderer)
    : renderer_(renderer) {}

void EditConsistSelectionHandler::onEnter() {
    renderer_.renderEditConsist();
}

void EditConsistSelectionHandler::onExit() {
    // Clean up state when exiting this mode
    menuCommandStarted = false;
    // Don't render here - let the next mode's onEnter handle rendering
}

bool EditConsistSelectionHandler::handle(const InputEvent &ev) {
    // Only handle key press events
    if (ev.type != InputEventType::KeypadChar && ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;
    
    switch (key) {
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9':
            {
                int selectedIndex = key - '0';
                int numLocos = wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar());
                
                // Check if selection is valid (0-based index must be < numLocos)
                if (selectedIndex < numLocos && numLocos > 1) {
                    String loco = wiThrottleProtocol.getLocomotiveAtPosition(throttleManager.getCurrentThrottleChar(), selectedIndex);
                    toggleLocoFacing(throttleManager.getCurrentThrottleIndex(), loco);
                }
                // Return to operation mode - onEnter will render speed
                inputManager.setMode(InputMode::Operation);
            }
            return true;
            
        case '*':  // cancel
            // Return to operation mode - onEnter will render speed
            inputManager.setMode(InputMode::Operation);
            return true;
            
        default:
            return false;
    }
}
