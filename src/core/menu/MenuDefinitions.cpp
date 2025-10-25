// MenuDefinitions.cpp - Menu structure and handlers
#include "MenuDefinitions.h"
#include "MenuSystem.h"
#include "../../../WiTcontroller.h"
#include "../../../static.h"
#include "../OledRenderer.h"
#include "../ThrottleManager.h"
#include "../heartbeat/HeartbeatMonitor.h"

// External dependencies
extern WiThrottleProtocol wiThrottleProtocol;
extern ThrottleManager throttleManager;
extern OledRenderer oledRenderer;
extern String turnoutPrefix;
extern String routePrefix;
extern bool dropBeforeAcquire;
extern bool hashShowsFunctionsInsteadOfKeyDefs;
extern HeartbeatMonitor heartbeatMonitor;

// Forward declarations from main sketch
extern void releaseAllLocos(int multiThrottleIndex);
extern void releaseOneLoco(int multiThrottleIndex, String loco);
extern void releaseOneLocoByIndex(int multiThrottleIndex, int index);
extern void toggleDirection(int multiThrottleIndex);
extern void cycleSpeedStep();
extern void powerToggle();
extern void writePreferences();
extern void disconnectWitServer();
extern void deepSleepStart();
extern void changeNumberOfThrottles(bool increase);
extern void toggleDropBeforeAquire();
extern String getLocoWithLength(String loco);
extern void doFunction(int, int, bool, bool);
extern void selectEditConsistList(int);
extern int witConnectionState;
extern bool preferencesRead;

// ==================== Menu Handlers ====================

namespace MenuHandlers {
    
    void handleAddLoco(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            if (dropBeforeAcquire && wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar()) > 0) {
                wiThrottleProtocol.releaseLocomotive(throttleManager.getCurrentThrottleChar(), "*");
            }
            String loco = getLocoWithLength(ctx.input);
            wiThrottleProtocol.addLocomotive(throttleManager.getCurrentThrottleChar(), loco);
            wiThrottleProtocol.getDirection(throttleManager.getCurrentThrottleChar(), loco);
            wiThrottleProtocol.getSpeed(throttleManager.getCurrentThrottleChar());
        } else {
            // No input - show roster list
            page = 0;
            oledRenderer.renderRoster("");
        }
    }
    
    void handleDropLoco(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            // Drop specific loco by address
            #ifndef CONSIST_RELEASE_BY_INDEX
                #define CONSIST_RELEASE_BY_INDEX false
            #endif
            if (!CONSIST_RELEASE_BY_INDEX) {
                String loco = getLocoWithLength(ctx.input);
                releaseOneLoco(throttleManager.getCurrentThrottleIndex(), loco);
            } else {
                releaseOneLocoByIndex(throttleManager.getCurrentThrottleIndex(), ctx.input.toInt());
            }
        } else {
            // No input - show list of current locos to select from
            oledRenderer.renderDropLocoList();
        }
    }
    
    void handleToggleDirection(MenuContext& ctx) {
        toggleDirection(throttleManager.getCurrentThrottleIndex());
    }
    
    void handleSpeedStep(MenuContext& ctx) {
        cycleSpeedStep();
    }
    
    void handleThrowPoint(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            String turnout = turnoutPrefix + ctx.input;
            wiThrottleProtocol.setTurnout(turnout, TurnoutThrow);
        } else {
            page = 0;
            oledRenderer.renderTurnoutList("", TurnoutThrow);
        }
    }
    
    void handleClosePoint(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            String turnout = turnoutPrefix + ctx.input;
            wiThrottleProtocol.setTurnout(turnout, TurnoutClose);
        } else {
            page = 0;
            oledRenderer.renderTurnoutList("", TurnoutClose);
        }
    }
    
    void handleRoute(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            String route = routePrefix + ctx.input;
            wiThrottleProtocol.setRoute(route);
        } else {
            page = 0;
            oledRenderer.renderRouteList("");
        }
    }
    
    void handleTrackPower(MenuContext& ctx) {
        powerToggle();
    }
    
    void handleFunction(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            // Direct function number entered
            int functionNumber = ctx.input.toInt();
            doFunction(throttleManager.getCurrentThrottleIndex(), functionNumber, true, true);
        } else {
            // Show function list
            functionPage = 0;
            oledRenderer.renderFunctionList("");
        }
    }
    
    // Extras submenu handlers
    void handleFunctionKeyToggle(MenuContext& ctx) {
        hashShowsFunctionsInsteadOfKeyDefs = !hashShowsFunctionsInsteadOfKeyDefs;
    }
    
    void handleEditConsist(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            char key = ctx.input.charAt(0);
            if (((key - '0') > 0) && ((key - '0') <= wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar()))) {
                selectEditConsistList(key - '0');
            }
        } else {
            oledRenderer.renderEditConsist();
        }
    }
    
    void handleHeartbeatToggle(MenuContext& ctx) {
        heartbeatMonitor.toggleEnabled();
        wiThrottleProtocol.requireHeartbeat(heartbeatMonitor.enabled());
        oledRenderer.renderHeartbeatCheck();
    }
    
    void handleIncreaseThrottles(MenuContext& ctx) {
        changeNumberOfThrottles(true);
    }
    
    void handleDecreaseThrottles(MenuContext& ctx) {
        changeNumberOfThrottles(false);
    }
    
    void handleDisconnect(MenuContext& ctx) {
        if (witConnectionState == 2) { // CONNECTION_STATE_CONNECTED
            witConnectionState = 0; // CONNECTION_STATE_DISCONNECTED
            preferencesRead = false;
            disconnectWitServer();
        }
    }
    
    void handleSleep(MenuContext& ctx) {
        deepSleepStart();
    }
    
    void handleDropBeforeAcquireToggle(MenuContext& ctx) {
        toggleDropBeforeAquire();
    }
    
    void handleSaveLocos(MenuContext& ctx) {
        writePreferences();
    }
    
    // List renderers (thin wrappers around existing functions)
    void renderRosterList() {
        page = 0;
        oledRenderer.renderRoster("");
    }
    
    void renderFunctionList() {
        functionPage = 0;
        oledRenderer.renderFunctionList("");
    }
    
    void renderTurnoutListThrow() {
        page = 0;
        oledRenderer.renderTurnoutList("", TurnoutThrow);
    }
    
    void renderTurnoutListClose() {
        page = 0;
        oledRenderer.renderTurnoutList("", TurnoutClose);
    }
    
    void renderRouteList() {
        page = 0;
        oledRenderer.renderRouteList("");
    }
    
    void renderDropLocoList() {
        oledRenderer.renderDropLocoList();
    }
}

// ==================== Menu Definitions ====================

namespace MenuDefinitions {
    
    // Extras submenu (replicates items 10-19 from menuText array)
    const MenuItem extrasMenuItems[] = {
        MenuItem::action(1, "Fn Key Toggle", "N/A",
                        MenuHandlers::handleFunctionKeyToggle),
        
        MenuItem::input(2, "Edt Consist", "# Select * Cancel",
                       MenuHandlers::handleEditConsist),
        
        MenuItem::action(3, "TBA", "N/A",
                        [](MenuContext& ctx){}), // Placeholder
        
        MenuItem::action(4, "Heartbt Tgl", "N/A",
                        MenuHandlers::handleHeartbeatToggle),
        
        MenuItem::action(5, "#Throttles +", "N/A",
                        MenuHandlers::handleIncreaseThrottles),
        
        MenuItem::action(6, "#Throttles -", "N/A",
                        MenuHandlers::handleDecreaseThrottles),
        
        MenuItem::action(7, "Disconnect", "N/A",
                        MenuHandlers::handleDisconnect),
        
        MenuItem::action(8, "OFF / Sleep", "N/A",
                        MenuHandlers::handleSleep),
        
        MenuItem::action(9, "1 Loco Tgl", "N/A",
                        MenuHandlers::handleDropBeforeAcquireToggle),
        
        MenuItem::action(0, "Save Locos", "N/A",
                        MenuHandlers::handleSaveLocos)
    };
    const uint8_t extrasMenuSize = 10;
    
    // Main menu (replicates items 0-9 from menuText array)
    // Items 1-9 followed by item 0 (Function) to match keypad layout
    const MenuItem mainMenuItems[] = {
        MenuItem::input(1, "Add Loco", "# End * Cancel",
                       MenuHandlers::handleAddLoco),
        
        MenuItem::list(2, "Drop Loco", "1-9 Select 0 All * Cancel",
                      nullptr,  // Handler not used for LIST items
                      MenuHandlers::renderDropLocoList),
        
        MenuItem::action(3, "Toggle Dir", "N/A",
                        MenuHandlers::handleToggleDirection),
        
        MenuItem::action(4, "X SpeedStep", "N/A",
                        MenuHandlers::handleSpeedStep),
        
        MenuItem::input(5, "Throw Point", "# List or Enter ID",
                       MenuHandlers::handleThrowPoint),
        
        MenuItem::input(6, "Close Point", "# List or Enter ID",
                       MenuHandlers::handleClosePoint),
        
        MenuItem::input(7, "Route", "# List or Enter ID",
                       MenuHandlers::handleRoute),
        
        MenuItem::action(8, "Track Power", "N/A",
                        MenuHandlers::handleTrackPower),
        
        MenuItem::submenu(9, "Extras", "0-9 Select * Cancel",
                         extrasMenuItems, extrasMenuSize),
        
        MenuItem::list(0, "Function", "0-9 Select * Cancel",
                      MenuHandlers::handleFunction,
                      MenuHandlers::renderFunctionList)
    };
    const uint8_t mainMenuSize = 10;
}
