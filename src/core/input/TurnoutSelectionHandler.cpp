#include "TurnoutSelectionHandler.h"
#include "../Renderer.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

TurnoutSelectionHandler::TurnoutSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer), action_(TurnoutThrow) {}

int TurnoutSelectionHandler::getItemCount() const {
    return turnoutListSize;
}

int TurnoutSelectionHandler::getItemsPerPage() const {
    return renderer_.getLayout().turnoutItemsPerPage;
}

String TurnoutSelectionHandler::getItemLabel(int gi, bool & /*invert*/) const {
    int nameMax = renderer_.getLayout().turnoutNameMaxLength;
    if (turnoutListUserName[gi].length() == 0) return "";
    String name = turnoutListUserName[gi];
    if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
    return name;
}

String TurnoutSelectionHandler::getFooterText() const {
    return "(" + String(getPage() + 1) + ") " + menu_text[menu_turnout_list];
}

void TurnoutSelectionHandler::onBeforeRender() {
    lastOledScreen = last_oled_screen_turnout_list;
    lastOledStringParameter = "";
    menuIsShowing = true;
}

void TurnoutSelectionHandler::onItemSelected(int index) {
    selectTurnoutList(index, action_);
}
