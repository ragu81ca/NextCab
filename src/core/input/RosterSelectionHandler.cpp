#include "RosterSelectionHandler.h"
#include "../Renderer.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

RosterSelectionHandler::RosterSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

int RosterSelectionHandler::getItemCount() const {
    return rosterSize;
}

int RosterSelectionHandler::getItemsPerPage() const {
    return renderer_.getLayout().rosterItemsPerPage;
}

String RosterSelectionHandler::getItemLabel(int gi, bool & /*invert*/) const {
    int nameMax = renderer_.getLayout().rosterNameMaxLength;
    int index = rosterSortedIndex[gi];
    if (rosterAddress[index] == 0) return "";
    String name = rosterName[index];
    if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
    return name + " (" + rosterAddress[index] + ")";
}

String RosterSelectionHandler::getFooterText() const {
    return "(" + String(getPage() + 1) + ") " + menu_text[menu_roster];
}

void RosterSelectionHandler::onBeforeRender() {
    lastOledScreen = last_oled_screen_roster;
    lastOledStringParameter = "";
    menuIsShowing = true;
}

void RosterSelectionHandler::onItemSelected(int index) {
    selectRoster(index);
}
