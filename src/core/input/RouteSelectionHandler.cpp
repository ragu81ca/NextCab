#include "RouteSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"
#include "../UIState.h"

extern InputManager inputManager;
extern UIState uiState;

RouteSelectionHandler::RouteSelectionHandler(Renderer &renderer)
    : renderer_(renderer), page_(0) {}

void RouteSelectionHandler::onEnter() {
    page_ = 0;
    uiState.page = 0;  // Sync with global state for renderer
    renderer_.renderRouteList("");
}

void RouteSelectionHandler::onExit() {
    page_ = 0;
    menuCommandStarted = false;
    // Don't render here - let the next mode's onEnter handle rendering
}

bool RouteSelectionHandler::handle(const InputEvent &ev) {
    // Only handle key press events
    if (ev.type != InputEventType::KeypadChar && ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;
    
    switch (key) {
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9': {
            int itemsPerPage = renderer_.getLayout().routeItemsPerPage;
            int index = ((key == '0') ? 9 : (key - '1')) + (page_ * itemsPerPage);
            selectRouteList(index);
            return true;
        }
            
        case '#':  // next page
            {
                int itemsPerPage = renderer_.getLayout().routeItemsPerPage;
                if (routeListSize > itemsPerPage) {
                    if ((page_ + 1) * itemsPerPage < routeListSize) {
                        page_++;
                    } else {
                        page_ = 0;
                    }
                    uiState.page = page_;  // Sync with global state for renderer
                    renderer_.renderRouteList("");
                }
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
