#include "TurnoutSelectionHandler.h"
#include "../Renderer.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

TurnoutSelectionHandler::TurnoutSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer), action_(TurnoutThrow) {}

void TurnoutSelectionHandler::configureScreen() {
    auto &s = screen();
    s.totalItems     = turnoutListSize;
    s.visibleRows    = renderer_.getLayout().turnoutItemsPerPage;
    s.halfPageSplit  = true;
    s.footerTemplate = "(%p) " + String(menu_text[menu_turnout_list]);

    s.itemLabel = [this](int gi, bool & /*invert*/) -> String {
        int nameMax = renderer_.getLayout().turnoutNameMaxLength;
        if (turnoutListUserName[gi].length() == 0) return "";
        String name = turnoutListUserName[gi];
        if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
        return name;
    };

    // Capture action_ by value so the lambda is self-contained
    TurnoutAction action = action_;
    s.onSelect = [action](int index) { selectTurnoutList(index, action); };

    s.onBeforeRender = []() {
        lastOledScreen = last_oled_screen_turnout_list;
        lastOledStringParameter = "";
        menuIsShowing = true;
    };
}
