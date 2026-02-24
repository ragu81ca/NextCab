#pragma once
#include "IModeHandler.h"
#include "../PagedListModel.h"

// Forward declarations
class Renderer;

/// Base class for selection handlers that display a paginated list of items
/// and allow the user to select one by pressing a digit key.
///
/// Digit keys map 1-based: '1' → index 0, '2' → index 1, … '9' → index 8, '0' → index 9.
/// '#' advances to the next page (wraps around).
/// '*' cancels (returns to Operation mode by default).
///
/// Subclasses must implement:
///   getItemCount()     — total items in the list
///   getItemsPerPage()  — how many items fit on one page (from DisplayLayout)
///   getItemLabel()     — return the display label for one global index
///   getFooterText()    — return the footer / navigation hint string
///   onItemSelected()   — act on the chosen absolute index
///
/// Subclasses may override:
///   onCancel()         — default sets InputMode::Operation
///   handleExtraKey()   — intercept keys before standard digit/page/cancel handling
///   syncPageState()    — default writes to uiState.page
///   useHalfPageSplit() — default false; true puts footer at halfPage row
///   onBeforeRender()   — set screen-tracking globals (lastOledScreen, menuIsShowing, etc.)
class PagedListHandler : public IModeHandler {
public:
    explicit PagedListHandler(Renderer &renderer);

    // IModeHandler — default implementations
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;

protected:
    // ── Required overrides ──────────────────────────────────────────────
    virtual int    getItemCount() const = 0;
    virtual int    getItemsPerPage() const = 0;
    virtual String getItemLabel(int globalIndex, bool &invert) const = 0;
    virtual String getFooterText() const = 0;
    virtual void   onItemSelected(int index) = 0;

    // ── Optional overrides ──────────────────────────────────────────────
    /// Called on '*'; default sets mode to Operation.
    virtual void onCancel();

    /// Called before standard key handling.  Return true if the key was consumed.
    virtual bool handleExtraKey(char key);

    /// Called after a page change.  Default sets uiState.page = page_.
    virtual void syncPageState(int page);

    /// Optional header text displayed above items (e.g. "Edit Consist Facing").
    virtual String getHeaderText() const { return ""; }

    /// Whether to use the half-page split layout (footer in middle row).
    virtual bool useHalfPageSplit() const { return false; }

    /// Called before rendering.  Set lastOledScreen, menuIsShowing, etc.
    virtual void onBeforeRender() {}

    // ── Accessors ───────────────────────────────────────────────────────
    int  getPage() const { return page_; }
    void setPage(int p)  { page_ = p; }

    Renderer &renderer_;
    int page_ = 0;

private:
    /// Template method: builds model from getItemLabel, calls renderer_.renderPagedList.
    void renderCurrentPage();
};
