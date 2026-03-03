#include "PagedListHandler.h"
#include "InputManager.h"
#include "../Renderer.h"
#include "../UIState.h"
#include "../../../WiTcontroller.h"

extern InputManager inputManager;
extern UIState uiState;

PagedListHandler::PagedListHandler(Renderer &renderer)
    : renderer_(renderer) {}

// ── Render current page from ListSelectionScreen ───────────────────────

void PagedListHandler::renderCurrentPage() {
    renderer_.renderListSelection(screen_);
}

// ── IModeHandler defaults ──────────────────────────────────────────────

void PagedListHandler::onEnter() {
    // Reset screen to blank state, then set common defaults
    screen_ = ListSelectionScreen();
    screen_.onCancel = []() {
        extern InputManager inputManager;
        inputManager.setMode(InputMode::Operation);
    };
    screen_.onPageChanged = [](int page) {
        extern UIState uiState;
        uiState.page = page;
    };

    // Let the subclass populate data source, callbacks, and hints
    configureScreen();

    // Sync page state and render
    if (screen_.onPageChanged) screen_.onPageChanged(0);
    renderCurrentPage();
}

void PagedListHandler::onExit() {
    screen_.reset();
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
            int offset;
            if (screen_.zeroIndexed) {
                offset = key - '0';  // 0-based: '0'→0, '1'→1, ..., '9'→9
            } else {
                offset = (key == '0') ? 9 : (key - '1');  // 1-based: '1'→0, ..., '0'→9
            }
            int index  = offset + (screen_.currentPage * screen_.visibleRows);
            if (screen_.onSelect) screen_.onSelect(index);
            return true;
        }

        // ── Next page ─────────────────────────────────────────────────
        case '#': {
            if (screen_.nextPage()) {
                if (screen_.onPageChanged) screen_.onPageChanged(screen_.currentPage);
                renderCurrentPage();
            }
            return true;
        }

        // ── Cancel / back ─────────────────────────────────────────────
        case '*':
            if (screen_.onCancel) screen_.onCancel();
            return true;

        default:
            return false;
    }
}

// ── Optional override defaults ─────────────────────────────────────────

bool PagedListHandler::handleExtraKey(char /*key*/) {
    return false;
}
