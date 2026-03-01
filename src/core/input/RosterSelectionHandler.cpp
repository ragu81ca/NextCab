#include "RosterSelectionHandler.h"
#include "../Renderer.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

RosterSelectionHandler::RosterSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

void RosterSelectionHandler::configureScreen() {
    auto &s = screen();
    s.totalItems    = rosterSize;
    s.visibleRows   = renderer_.getLayout().rosterItemsPerPage;
    s.footerTemplate = "(%p) " + String(menu_text[menu_roster]);

    s.itemLabel = [this](int gi, bool & /*invert*/) -> String {
        int nameMax = renderer_.getLayout().rosterNameMaxLength;
        int index = rosterSortedIndex[gi];
        if (rosterAddress[index] == 0) return "";
        String name = rosterName[index];
        if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
        return name + " (" + rosterAddress[index] + ")";
    };

    s.onSelect = [](int index) { selectRoster(index); };

    s.onBeforeRender = []() {
        lastOledScreen = last_oled_screen_roster;
        lastOledStringParameter = "";
        menuIsShowing = true;
    };
}
