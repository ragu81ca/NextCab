// Renderer.h - display-agnostic renderer (renamed from OledRenderer.h)
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h> // for TurnoutAction
#include "DisplayDriver.h"
#include "DisplayLayout.h"
#include "FontSet.h"
#include "PagedListModel.h"

class Renderer {
public:
    Renderer(DisplayDriver &driver, const DisplayLayout &layout, const FontSet &fonts);

    // Public rendering entry points used by application code
    void renderFoundSsids(const String &soFar);
    void renderEnterPassword();
    void renderHeartbeatCheck();
    void renderDropLocoList();  // Show current locos for dropping (1-based display)
    void renderNewMenu(class MenuSystem& menuSystem);  // New table-driven menu system rendering
    void renderSpeed();
    void renderDirectCommands();
    void renderBattery();
    void renderSpeedStepMultiplier();
    void renderMomentumIndicator();
    void renderBrakeIndicator();
    void renderArray(bool isThreeColumns, bool isPassword, bool sendBuffer=true, bool drawTopLine=false);
    void clearArray();
    // Public wrapper that renders and records the all-locos screen (replaces former public renderAllLocos)
    void renderAllLocosScreen(bool hideLeadLoco);

    /// Draw a paginated list from a pre-built model. Domain-free — the handler
    /// builds the items, the Renderer just draws them.
    void renderPagedList(const PagedListModel &model);

    // Access layout for code that needs content capacity values
    const DisplayLayout& getLayout() const { return layout; }

private:
    void renderArrayInternal(bool isThreeColumns, bool isPassword, bool sendBuffer, bool drawTopLine);
    void renderFunctions();
    void renderAllLocos(bool hideLeadLoco); // now private again
    
    // Helper functions for unified loco list rendering
    bool checkNeedSuffixes(char throttleChar, int numLocos);
    String formatLocoDisplay(const String &loco, bool needSuffixes);

    DisplayDriver &display;
    const DisplayLayout &layout;
    const FontSet &fonts;
};
