#include "FunctionSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../ThrottleManager.h"
#include "../UIState.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"
#include "../ui/TitleScreen.h"

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
        TitleScreen ts;
        ts.title = MSG_NO_FUNCTIONS;
        int currentIdx = throttleManager.getCurrentThrottleIndex();
        ts.addBody(MSG_THROTTLE_NUMBER + String(currentIdx + 1));
        ts.addBody(MSG_NO_LOCO_SELECTED);
        ts.footerText = menu_text[menu_cancel];
        ts.showTopLine = false;
        renderer_.renderTitle(ts);
        return;
    }
    // Normal path: delegate to base class
    PagedListHandler::onEnter();
}

void FunctionSelectionHandler::configureScreen() {
    auto &s = screen();
    s.totalItems     = MAX_FUNCTIONS;
    s.visibleRows    = renderer_.getLayout().functionItemsPerPage;
    s.halfPageSplit  = true;
    s.footerTemplate = "(%p) " + String(menu_text[menu_function_list]);

    s.itemLabel = [this](int gi, bool &invert) -> String {
        int currentIdx = throttleManager.getCurrentThrottleIndex();
        int labelMax = renderer_.getLayout().functionLabelMaxLength;
        String label = throttleManager.getFunctionLabel(currentIdx, gi);
        if (labelMax > 0 && (int)label.length() > labelMax) label = label.substring(0, labelMax);
        if (throttleManager.getFunctionState(currentIdx, gi)) invert = true;
        // Always include function number so the row is never empty
        return (label.length() > 0) ? (String(gi) + "-" + label) : String(gi);
    };

    s.onSelect = [](int index) {
        selectFunctionList(index);
        // Return to operation mode so encoder can control speed
        extern InputManager inputManager;
        inputManager.setMode(InputMode::Operation);
    };

    s.onBeforeRender = []() {
        lastOledScreen = last_oled_screen_function_list;
        lastOledStringParameter = "";
        menuIsShowing = true;
        extern UIState uiState;
        uiState.functionHasBeenSelected = false;
    };

    // Functions track their own page variable
    s.onPageChanged = [](int page) {
        extern UIState uiState;
        uiState.functionPage = page;
    };
}
