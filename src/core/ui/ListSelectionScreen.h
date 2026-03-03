// ListSelectionScreen.h — Data-only model for a paginated list screen.
//
// The handler populates this screen with a data source (itemLabel callback),
// action callbacks (onSelect, onCancel), and presentation hints (header, footer,
// halfPageSplit).  The Renderer reads from it to draw.
//
// Key principle: the screen knows NOTHING about pixels or fonts.
// The Renderer sets `visibleRows` after measuring available display space.
#pragma once

#include "IScreen.h"
#include <functional>

class ListSelectionScreen : public IScreen {
public:
    // ── Callback types ──────────────────────────────────────────────────
    /// Return the display label for `globalIndex`.  Set `invert` to true if the
    /// item should be rendered inverted (e.g. active function, reverse-facing loco).
    using ItemLabelFn    = std::function<String(int globalIndex, bool &invert)>;

    /// Called when the user selects item at `globalIndex`.
    using SelectFn       = std::function<void(int globalIndex)>;

    /// Called when the user cancels / presses '*'.
    using CancelFn       = std::function<void()>;

    /// Called just before rendering (set lastOledScreen, menuIsShowing, etc.).
    using BeforeRenderFn = std::function<void()>;

    /// Called after a page change so the handler can sync external state.
    using PageChangedFn  = std::function<void(int page)>;

    // ── IScreen ─────────────────────────────────────────────────────────
    ScreenType type() const override { return ScreenType::ListSelection; }

    // ── Configuration (set once by handler in configureScreen) ──────────
    int    totalItems     = 0;
    String headerText;             // optional header displayed above items
    String footerTemplate;         // use %p for page, %t for total pages
    bool   halfPageSplit  = false;  // put footer in middle row, items above & below
    bool   zeroIndexed    = false;  // when true, key '0'→item 0 and labels start at 0

    // Data source — called by Renderer to get label for each visible item
    ItemLabelFn    itemLabel;

    // Action callbacks
    SelectFn       onSelect;
    CancelFn       onCancel;
    BeforeRenderFn onBeforeRender;
    PageChangedFn  onPageChanged;

    // ── Renderer-controlled ─────────────────────────────────────────────
    int visibleRows = 5;           // set by handler from DisplayLayout; Renderer may override

    // ── State ───────────────────────────────────────────────────────────
    int currentPage = 0;

    // ── Helpers ─────────────────────────────────────────────────────────
    int pageCount() const {
        if (visibleRows <= 0 || totalItems <= 0) return 1;
        return (totalItems + visibleRows - 1) / visibleRows;
    }

    String footerText() const {
        String f = footerTemplate;
        f.replace("%p", String(currentPage + 1));
        f.replace("%t", String(pageCount()));
        return f;
    }

    /// Advance to next page (wraps).  Returns true if page actually changed.
    bool nextPage() {
        if (totalItems > visibleRows) {
            if ((currentPage + 1) * visibleRows < totalItems) {
                currentPage++;
            } else {
                currentPage = 0;
            }
            return true;
        }
        return false;
    }

    void reset() { currentPage = 0; }

    /// Map a 0-based visible-row index to the global item index on current page.
    int globalIndex(int visibleIndex) const {
        return currentPage * visibleRows + visibleIndex;
    }
};
