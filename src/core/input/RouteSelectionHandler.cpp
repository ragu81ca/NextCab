#include "RouteSelectionHandler.h"
#include "../Renderer.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

RouteSelectionHandler::RouteSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

int RouteSelectionHandler::getItemCount() const {
    return routeListSize;
}

int RouteSelectionHandler::getItemsPerPage() const {
    return renderer_.getLayout().routeItemsPerPage;
}

String RouteSelectionHandler::getItemLabel(int gi, bool & /*invert*/) const {
    int nameMax = renderer_.getLayout().routeNameMaxLength;
    if (routeListUserName[gi].length() == 0) return "";
    String name = routeListUserName[gi];
    if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
    return name;
}

String RouteSelectionHandler::getFooterText() const {
    return "(" + String(getPage() + 1) + ") " + menu_text[menu_route_list];
}

void RouteSelectionHandler::onBeforeRender() {
    lastOledScreen = last_oled_screen_route_list;
    lastOledStringParameter = "";
    menuIsShowing = true;
}

void RouteSelectionHandler::onItemSelected(int index) {
    selectRouteList(index);
}
