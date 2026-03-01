#include "RouteSelectionHandler.h"
#include "../Renderer.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

RouteSelectionHandler::RouteSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

void RouteSelectionHandler::configureScreen() {
    auto &s = screen();
    s.totalItems     = routeListSize;
    s.visibleRows    = renderer_.getLayout().routeItemsPerPage;
    s.halfPageSplit  = true;
    s.footerTemplate = "(%p) " + String(menu_text[menu_route_list]);

    s.itemLabel = [this](int gi, bool & /*invert*/) -> String {
        int nameMax = renderer_.getLayout().routeNameMaxLength;
        if (routeListUserName[gi].length() == 0) return "";
        String name = routeListUserName[gi];
        if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
        return name;
    };

    s.onSelect = [](int index) { selectRouteList(index); };

    s.onBeforeRender = []() {
        lastOledScreen = last_oled_screen_route_list;
        lastOledStringParameter = "";
        menuIsShowing = true;
    };
}
