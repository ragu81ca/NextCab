#include "TurnoutSelectionHandler.h"
#include "../Renderer.h"
#include "../../../WiTcontroller.h"

TurnoutSelectionHandler::TurnoutSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer), action_(TurnoutThrow) {}

int TurnoutSelectionHandler::getItemCount() const {
    return turnoutListSize;
}

int TurnoutSelectionHandler::getItemsPerPage() const {
    return renderer_.getLayout().turnoutItemsPerPage;
}

void TurnoutSelectionHandler::renderCurrentPage() {
    renderer_.renderTurnoutList("", action_);
}

void TurnoutSelectionHandler::onItemSelected(int index) {
    selectTurnoutList(index, action_);
}
