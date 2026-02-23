#include "RosterSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../UIState.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern UIState uiState;

RosterSelectionHandler::RosterSelectionHandler(Renderer &renderer)
    : renderer_(renderer), page_(0) {}

void RosterSelectionHandler::onEnter() {
    page_ = 0;
    uiState.page = 0;  // Sync with global state for renderer
    renderer_.renderRoster("");
}

void RosterSelectionHandler::onExit() {
    page_ = 0;
    menuCommandStarted = false;
    // Don't render here - let the next mode's onEnter handle rendering
}

bool RosterSelectionHandler::handle(const InputEvent &ev) {
    // Only handle key press events
    if (ev.type != InputEventType::KeypadChar && ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;
    
    switch (key) {
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9':
            selectRoster((key - '0') + (page_ * 5));
            return true;
            
        case '#':  // next page
            if (rosterSize > 5) {
                if ((page_ + 1) * 5 < rosterSize) {
                    page_++;
                } else {
                    page_ = 0;
                }
                uiState.page = page_;  // Sync with global state for renderer
                renderer_.renderRoster("");
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
