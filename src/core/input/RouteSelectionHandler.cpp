#include "RouteSelectionHandler.h"
#include "../Renderer.h"
#include "../../../WiTcontroller.h"

RouteSelectionHandler::RouteSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

int RouteSelectionHandler::getItemCount() const {
    return routeListSize;
}

int RouteSelectionHandler::getItemsPerPage() const {
    return renderer_.getLayout().routeItemsPerPage;
}

void RouteSelectionHandler::renderCurrentPage() {
    renderer_.renderRouteList("");
}

void RouteSelectionHandler::onItemSelected(int index) {
    selectRouteList(index);
}
