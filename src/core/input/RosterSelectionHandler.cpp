#include "RosterSelectionHandler.h"
#include "../Renderer.h"
#include "../ServerDataStore.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern ServerDataStore serverDataStore;

RosterSelectionHandler::RosterSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

void RosterSelectionHandler::configureScreen() {
    auto &s = screen();
    s.totalItems    = serverDataStore.rosterSize();
    s.visibleRows   = renderer_.getLayout().rosterItemsPerPage;
    s.footerTemplate = "(%p) " + String(menu_text[menu_roster]);

    s.itemLabel = [this](int gi, bool & /*invert*/) -> String {
        int nameMax = renderer_.getLayout().rosterNameMaxLength;
        int index = serverDataStore.rosterSortedIndex(gi);
        if (serverDataStore.rosterAddress(index) == 0) return "";
        String name = serverDataStore.rosterName(index);
        if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
        return name + " (" + serverDataStore.rosterAddress(index) + ")";
    };

    s.onSelect = [](int index) { selectRoster(index); };

    s.onBeforeRender = []() {
        lastOledScreen = last_oled_screen_roster;
        lastOledStringParameter = "";
        menuIsShowing = true;
    };
}
