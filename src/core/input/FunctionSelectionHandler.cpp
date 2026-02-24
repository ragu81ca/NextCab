#include "FunctionSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../ThrottleManager.h"
#include "../UIState.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern UIState uiState;
extern ThrottleManager throttleManager;
extern WiThrottleProtocol wiThrottleProtocol;

FunctionSelectionHandler::FunctionSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

void FunctionSelectionHandler::onEnter() {
    char currentChar = throttleManager.getCurrentThrottleChar();
    if (wiThrottleProtocol.getNumberOfLocomotives(currentChar) == 0) {
        // No loco selected — show error message and let '*' cancel out
        lastOledScreen = last_oled_screen_function_list;
        menuIsShowing = true;
        renderer_.clearArray();
        oledText[0] = MSG_NO_FUNCTIONS;
        int currentIdx = throttleManager.getCurrentThrottleIndex();
        oledText[2] = MSG_THROTTLE_NUMBER + String(currentIdx + 1);
        oledText[3] = MSG_NO_LOCO_SELECTED;
        setMenuTextForOled(menu_cancel);
        renderer_.renderArray(false, false, true, false);
        return;
    }
    // Normal path: delegate to base class
    PagedListHandler::onEnter();
}

int FunctionSelectionHandler::getItemCount() const {
    return MAX_FUNCTIONS;
}

int FunctionSelectionHandler::getItemsPerPage() const {
    return renderer_.getLayout().functionItemsPerPage;
}

String FunctionSelectionHandler::getItemLabel(int gi, bool &invert) const {
    int currentIdx = throttleManager.getCurrentThrottleIndex();
    int labelMax = renderer_.getLayout().functionLabelMaxLength;
    String label = functionLabels[currentIdx][gi];
    if (labelMax > 0 && (int)label.length() > labelMax) label = label.substring(0, labelMax);
    if (functionStates[currentIdx][gi]) invert = true;
    return (gi < 10) ? label : String(gi) + "-" + label;
}

String FunctionSelectionHandler::getFooterText() const {
    return "(" + String(getPage()) + ") " + menu_text[menu_function_list];
}

void FunctionSelectionHandler::onBeforeRender() {
    lastOledScreen = last_oled_screen_function_list;
    lastOledStringParameter = "";
    menuIsShowing = true;
    uiState.functionHasBeenSelected = false;
}

void FunctionSelectionHandler::onItemSelected(int index) {
    selectFunctionList(index);
    // Return to operation mode so encoder can control speed
    inputManager.setMode(InputMode::Operation);
}

void FunctionSelectionHandler::syncPageState(int page) {
    uiState.functionPage = page;
}
