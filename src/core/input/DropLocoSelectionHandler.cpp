#include "DropLocoSelectionHandler.h"
#include "InputManager.h"
#include "../OledRenderer.h"
#include "../ThrottleManager.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern ThrottleManager throttleManager;
extern WiThrottleProtocol wiThrottleProtocol;

DropLocoSelectionHandler::DropLocoSelectionHandler(OledRenderer &renderer)
    : renderer_(renderer) {}

void DropLocoSelectionHandler::onEnter() {
    keypadUseType = KEYPAD_USE_SELECT_DROP_LOCO;
    renderer_.renderDropLocoList();
}

void DropLocoSelectionHandler::onExit() {
    // Clean up state when exiting this mode
    keypadUseType = KEYPAD_USE_OPERATION;
    menuCommandStarted = false;
    // Don't render here - let the next mode's onEnter handle rendering
}

bool DropLocoSelectionHandler::handle(const InputEvent &ev) {
    // Only handle key press events
    if (ev.type != InputEventType::KeypadChar && ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;
    
    switch (key) {
        case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9':
            // Convert 1-based display to 0-based index
            if ((key-'0') <= wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar())) {
                releaseOneLocoByIndex(throttleManager.getCurrentThrottleIndex(), key - '0' - 1);
                // Return to operation mode - onEnter will render speed
                inputManager.setMode(InputMode::Operation);
            }
            return true;
            
        case '0':  // Drop all locos
            releaseAllLocos(throttleManager.getCurrentThrottleIndex());
            // Return to operation mode - onEnter will render speed
            inputManager.setMode(InputMode::Operation);
            return true;
            
        case '*':  // cancel
            // Return to operation mode - onEnter will render speed
            inputManager.setMode(InputMode::Operation);
            return true;
            
        default:
            return false;
    }
}
