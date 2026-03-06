// Renderer.h - display-agnostic renderer (renamed from OledRenderer.h)
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h> // for TurnoutAction
#include "DisplayDriver.h"
#include "DisplayLayout.h"
#include "FontSet.h"
#include "PagedListModel.h"
#include "ui/ListSelectionScreen.h"
#include "ui/TitleScreen.h"
#include "ui/WaitScreen.h"
#include "ui/TextInputScreen.h"
#include "ui/ThrottleScreen.h"

class Renderer {
public:
    Renderer(DisplayDriver &driver, const DisplayLayout &layout, const FontSet &fonts);

    // Public rendering entry points used by application code
    void renderHeartbeatCheck();
    void renderNewMenu(class MenuSystem& menuSystem);  // New table-driven menu system rendering
    void renderSpeed();
    void renderDirectCommands();
    void renderBattery();
    void renderArray(bool isThreeColumns, bool isPassword, bool sendBuffer=true, bool drawTopLine=false);
    void clearArray();
    // Public wrapper that renders and records the all-locos screen (replaces former public renderAllLocos)
    void renderAllLocosScreen(bool hideLeadLoco);

    /// Draw a paginated list from a pre-built model. Domain-free — the handler
    /// builds the items, the Renderer just draws them.
    void renderPagedList(const PagedListModel &model);

    /// Render a ListSelectionScreen directly.  The Renderer calls the screen's
    /// itemLabel callback to fetch display data and handles all spatial layout.
    void renderListSelection(ListSelectionScreen &screen);

    /// Render a TitleScreen — header, vertically-centred body, optional footer.
    void renderTitle(const TitleScreen &screen);

    /// Render a WaitScreen — same layout as TitleScreen plus a bouncing-dot
    /// spinner drawn below the body text.
    void renderWait(const WaitScreen &screen);

    /// Render a TextInputScreen — prompt lines, input with blinking caret, footer.
    void renderTextInput(const TextInputScreen &screen);

    /// Render the main throttle operating screen from a pre-built model.
    /// Replaces the old renderSpeed() which reached into globals directly.
    void renderThrottleScreen(const ThrottleScreen &screen);

    /// Populate a ThrottleScreen model from current system state.
    /// Formerly a free function in WiTcontroller.ino; now a Renderer member
    /// since it directly populates oledText[] for the menu footer.
    void buildThrottleScreen(ThrottleScreen &screen);

    // Access layout for code that needs content capacity values
    const DisplayLayout& getLayout() const { return layout; }

    // Helper functions for loco display (used by DropLocoSelectionHandler, EditConsistSelectionHandler)
    bool checkNeedSuffixes(char throttleChar, int numLocos);
    String formatLocoDisplay(const String &loco, bool needSuffixes);

private:
    void renderArrayInternal(bool isThreeColumns, bool isPassword, bool sendBuffer, bool drawTopLine, bool centerText = false, bool fullWidthInvert = false);
    int  populateTitleArray(const TitleScreen &screen); // shared layout for renderTitle/renderWait
    void drawSpinner(int centerY, int frame);            // bouncing-dot animation
    void renderFunctions();
    void renderAllLocos(bool hideLeadLoco); // now private again

    DisplayDriver &display;
    const DisplayLayout &layout;
    const FontSet &fonts;
};
