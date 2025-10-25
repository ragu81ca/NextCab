// MenuDefinitions.h - Declarative menu structure
#pragma once

#include "MenuItem.h"

// Forward declarations for handler functions
namespace MenuHandlers {
    void handleAddLoco(MenuContext& ctx);
    void handleDropLoco(MenuContext& ctx);
    void handleToggleDirection(MenuContext& ctx);
    void handleSpeedStep(MenuContext& ctx);
    void handleThrowPoint(MenuContext& ctx);
    void handleClosePoint(MenuContext& ctx);
    void handleRoute(MenuContext& ctx);
    void handleTrackPower(MenuContext& ctx);
    void handleFunction(MenuContext& ctx);
    
    // Extras submenu handlers
    void handleFunctionKeyToggle(MenuContext& ctx);
    void handleEditConsist(MenuContext& ctx);
    void handleHeartbeatToggle(MenuContext& ctx);
    void handleIncreaseThrottles(MenuContext& ctx);
    void handleDecreaseThrottles(MenuContext& ctx);
    void handleDisconnect(MenuContext& ctx);
    void handleSleep(MenuContext& ctx);
    void handleDropBeforeAcquireToggle(MenuContext& ctx);
    void handleSaveLocos(MenuContext& ctx);
    
    // List renderers
    void renderRosterList();
    void renderFunctionList();
    void renderTurnoutListThrow();
    void renderTurnoutListClose();
    void renderRouteList();
}

// Menu structure definitions
namespace MenuDefinitions {
    // Extras submenu items (items 10-19 from menuText array)
    extern const MenuItem extrasMenuItems[];
    extern const uint8_t extrasMenuSize;
    
    // Main menu items (items 0-9 from menuText array)
    extern const MenuItem mainMenuItems[];
    extern const uint8_t mainMenuSize;
}
