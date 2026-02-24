#include "RosterSelectionHandler.h"
#include "../Renderer.h"
#include "../../../WiTcontroller.h"

RosterSelectionHandler::RosterSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

int RosterSelectionHandler::getItemCount() const {
    return rosterSize;
}

int RosterSelectionHandler::getItemsPerPage() const {
    return renderer_.getLayout().rosterItemsPerPage;
}

void RosterSelectionHandler::renderCurrentPage() {
    renderer_.renderRoster("");
}

void RosterSelectionHandler::onItemSelected(int index) {
    selectRoster(index);
}
