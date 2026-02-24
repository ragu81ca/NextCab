#include "FunctionSelectionHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../UIState.h"
#include "../../../static.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern UIState uiState;

FunctionSelectionHandler::FunctionSelectionHandler(Renderer &renderer)
    : PagedListHandler(renderer) {}

int FunctionSelectionHandler::getItemCount() const {
    return MAX_FUNCTIONS;
}

int FunctionSelectionHandler::getItemsPerPage() const {
    return renderer_.getLayout().functionItemsPerPage;
}

void FunctionSelectionHandler::renderCurrentPage() {
    renderer_.renderFunctionList("");
}

void FunctionSelectionHandler::onItemSelected(int index) {
    selectFunctionList(index);
    // Return to operation mode so encoder can control speed
    inputManager.setMode(InputMode::Operation);
}

void FunctionSelectionHandler::syncPageState(int page) {
    uiState.functionPage = page;
}
