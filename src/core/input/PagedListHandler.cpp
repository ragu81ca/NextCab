#include "PagedListHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../UIState.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern UIState uiState;

PagedListHandler::PagedListHandler(Renderer &renderer)
    : renderer_(renderer) {}

// ── Template method: builds model and delegates to Renderer ────────────

void PagedListHandler::renderCurrentPage() {
    onBeforeRender();

    PagedListModel model;
    model.halfPageSplit = useHalfPageSplit();
    model.headerText    = getHeaderText();
    model.footerText    = getFooterText();

    int perPage = getItemsPerPage();
    model.pageCapacity  = perPage;
    int count   = getItemCount();
    for (int i = 0; i < perPage; i++) {
        int gi = page_ * perPage + i;
        if (gi >= count) break;
        bool invert = false;
        String label = getItemLabel(gi, invert);
        model.addItem(label, invert);
    }

    renderer_.renderPagedList(model);
}

// ── IModeHandler defaults ──────────────────────────────────────────────

void PagedListHandler::onEnter() {
    page_ = 0;
    syncPageState(0);
    renderCurrentPage();
}

void PagedListHandler::onExit() {
    page_ = 0;
    menuCommandStarted = false;
}

bool PagedListHandler::handle(const InputEvent &ev) {
    if (ev.type != InputEventType::KeypadChar &&
        ev.type != InputEventType::KeypadSpecial) {
        return false;
    }

    char key = ev.cvalue;

    // Let subclass intercept first (e.g. rescan, address entry)
    if (handleExtraKey(key)) {
        return true;
    }

    switch (key) {
        // ── Item selection (1-based: '1'→0 … '9'→8, '0'→9) ───────────
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': {
            int offset = (key == '0') ? 9 : (key - '1');
            int index  = offset + (page_ * getItemsPerPage());
            onItemSelected(index);
            return true;
        }

        // ── Next page ─────────────────────────────────────────────────
        case '#': {
            int perPage = getItemsPerPage();
            int count   = getItemCount();
            if (count > perPage) {
                if ((page_ + 1) * perPage < count) {
                    page_++;
                } else {
                    page_ = 0;
                }
                syncPageState(page_);
                renderCurrentPage();
            }
            return true;
        }

        // ── Cancel / back ─────────────────────────────────────────────
        case '*':
            onCancel();
            return true;

        default:
            return false;
    }
}

// ── Optional override defaults ─────────────────────────────────────────

void PagedListHandler::onCancel() {
    inputManager.setMode(InputMode::Operation);
}

bool PagedListHandler::handleExtraKey(char /*key*/) {
    return false;
}

void PagedListHandler::syncPageState(int page) {
    uiState.page = page;
}
