#pragma once
#include "IModeHandler.h"
#include "../ui/ListSelectionScreen.h"

// Forward declarations
class Renderer;

/// Base class for selection handlers that display a paginated list of items
/// and allow the user to select one by pressing a digit key.
///
/// Digit keys map 1-based: '1' → index 0, '2' → index 1, … '9' → index 8, '0' → index 9.
/// '#' advances to the next page (wraps around).
/// '*' cancels (returns to Operation mode by default).
///
/// Subclasses implement configureScreen() to populate the ListSelectionScreen
/// with a data source, action callbacks, and display hints.
///
/// Subclasses may override:
///   handleExtraKey()   — intercept keys before standard digit/page/cancel handling
class PagedListHandler : public IModeHandler {
public:
    explicit PagedListHandler(Renderer &renderer);

    // IModeHandler — default implementations
    bool handle(const InputEvent &ev) override;
    void onEnter() override;
    void onExit() override;

protected:
    /// Subclasses populate screen_ with data source, callbacks, and hints.
    /// Called from onEnter() after screen_ is reset to defaults.
    virtual void configureScreen() = 0;

    /// Called before standard key handling.  Return true if the key was consumed.
    virtual bool handleExtraKey(char key);

    // ── Accessors ───────────────────────────────────────────────────────
    ListSelectionScreen& screen()  { return screen_; }
    const ListSelectionScreen& screen() const { return screen_; }

    Renderer &renderer_;
    ListSelectionScreen screen_;

    /// Renders the current page via the Renderer.
    void renderCurrentPage();
};
