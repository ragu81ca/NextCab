#include "FunctionSelectionHandler.h"
#include "InputManager.h"
#include "../OledRenderer.h"
#include "../UIState.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern UIState uiState;

FunctionSelectionHandler::FunctionSelectionHandler(OledRenderer &renderer)
    : renderer_(renderer), functionPage_(0) {}

void FunctionSelectionHandler::onEnter() {
    functionPage_ = 0;
    uiState.functionPage = 0;  // Sync with global state for renderer
    renderer_.renderFunctionList("");
}

void FunctionSelectionHandler::onExit() {
    functionPage_ = 0;
    menuCommandStarted = false;
    // Don't render here - let the next mode's onEnter handle rendering
}

bool FunctionSelectionHandler::handle(const InputEvent &ev) {
    // Only handle key press events
    if (ev.type != InputEventType::KeypadChar && ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;
    
    switch (key) {
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9':
            selectFunctionList((key - '0') + (functionPage_ * 10));
            // Return to operation mode so encoder can control speed
            inputManager.setMode(InputMode::Operation);
            return true;
            
        case '#':  // next page
            if ((functionPage_ + 1) * 10 < MAX_FUNCTIONS) {
                functionPage_++;
                uiState.functionPage = functionPage_;  // Sync with global state for renderer
                renderer_.renderFunctionList("");
            } else {
                // Wrap back to first page
                functionPage_ = 0;
                uiState.functionPage = 0;
                renderer_.renderFunctionList("");
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
