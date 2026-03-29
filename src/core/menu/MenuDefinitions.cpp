// MenuDefinitions.cpp - Menu structure and handlers
#include "MenuDefinitions.h"
#include "MenuSystem.h"
#include "../../../WiTcontroller.h"
#include "../../../static.h"
#include "../Renderer.h"
#include "../ThrottleManager.h"
#include "../ServerDataStore.h"
#include "../LocoManager.h"
#include "../heartbeat/HeartbeatMonitor.h"
#include "../input/InputManager.h"
#include "../input/TurnoutSelectionHandler.h"
#include "../SystemState.h"
#include "../network/WiThrottleConnectionManager.h"

// External dependencies
extern WiThrottleProtocol wiThrottleProtocol;
extern ThrottleManager throttleManager;
extern Renderer renderer;
extern InputManager inputManager;
extern TurnoutSelectionHandler turnoutSelectionHandler;
extern ServerDataStore serverDataStore;
extern LocoManager locoManager;
extern bool hashShowsFunctionsInsteadOfKeyDefs;
extern HeartbeatMonitor heartbeatMonitor;
extern WiThrottleConnectionManager connectionManager;
extern TrackPower trackPower;

// Forward declarations from main sketch
extern void deepSleepStart();
extern SystemStateManager systemStateManager;

// ==================== Menu Handlers ====================

namespace MenuHandlers {
    
    void handleAddLoco(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            String loco = locoManager.getLocoWithLength(ctx.input);
            locoManager.acquireLoco(throttleManager.getCurrentThrottleIndex(), loco);
        } else {
            // No input - show roster list via InputManager
            inputManager.setMode(InputMode::RosterSelection);
        }
    }
    
    void handleDropLoco(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            // Drop specific loco by address
            #ifndef CONSIST_RELEASE_BY_INDEX
                #define CONSIST_RELEASE_BY_INDEX false
            #endif
            if (!CONSIST_RELEASE_BY_INDEX) {
                String loco = locoManager.getLocoWithLength(ctx.input);
                locoManager.releaseOneLoco(throttleManager.getCurrentThrottleIndex(), loco);
            } else {
                locoManager.releaseOneLocoByIndex(throttleManager.getCurrentThrottleIndex(), ctx.input.toInt());
            }
        } else {
            // No input - show list of current locos to select from via InputManager
            inputManager.setMode(InputMode::DropLocoSelection);
        }
    }
    
    void handleToggleDirection(MenuContext& ctx) {
        throttleManager.toggleDirection(throttleManager.getCurrentThrottleIndex());
    }
    
    void handleSpeedStep(MenuContext& ctx) {
        throttleManager.cycleSpeedStep();
    }
    
    void handleThrowPoint(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            String turnout = serverDataStore.turnoutPrefix() + ctx.input;
            wiThrottleProtocol.setTurnout(turnout, TurnoutThrow);
        } else {
            // No input - show turnout list via InputManager
            turnoutSelectionHandler.setAction(TurnoutThrow);
            inputManager.setMode(InputMode::TurnoutSelection);
        }
    }
    
    void handleClosePoint(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            String turnout = serverDataStore.turnoutPrefix() + ctx.input;
            wiThrottleProtocol.setTurnout(turnout, TurnoutClose);
        } else {
            // No input - show turnout list via InputManager
            turnoutSelectionHandler.setAction(TurnoutClose);
            inputManager.setMode(InputMode::TurnoutSelection);
        }
    }
    
    void handleRoute(MenuContext& ctx) {
        if (ctx.input.length() > 0) {
            String route = serverDataStore.routePrefix() + ctx.input;
            wiThrottleProtocol.setRoute(route);
        } else {
            // No input - show route list via InputManager
            inputManager.setMode(InputMode::RouteSelection);
        }
    }
    
    void handleTrackPower(MenuContext& ctx) {
        if (trackPower == PowerOn) {
            wiThrottleProtocol.setTrackPower(PowerOff);
            trackPower = PowerOff;
        } else {
            wiThrottleProtocol.setTrackPower(PowerOn);
            trackPower = PowerOn;
        }
        renderer.renderSpeed();
    }
    
    void handleFunction(MenuContext& ctx) {
        if (wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar()) == 0) return;
        if (ctx.input.length() > 0) {
            // Direct function number entered
            int functionNumber = ctx.input.toInt();
            throttleManager.toggleFunction(throttleManager.getCurrentThrottleIndex(), functionNumber, true, true);
        } else {
            // Show function list via InputManager
            inputManager.setMode(InputMode::FunctionSelection);
        }
    }
    
    // Extras submenu handlers
    void handleFunctionKeyToggle(MenuContext& ctx) {
        hashShowsFunctionsInsteadOfKeyDefs = !hashShowsFunctionsInsteadOfKeyDefs;
    }
    
    void handleEditConsist(MenuContext& ctx) {
        // Show list via InputManager
        inputManager.setMode(InputMode::EditConsist);
    }
    
    void handleHeartbeatToggle(MenuContext& ctx) {
        heartbeatMonitor.toggleEnabled();
        wiThrottleProtocol.requireHeartbeat(heartbeatMonitor.enabled());
        renderer.renderHeartbeatCheck();
    }
    
    void handleMomentumToggle(MenuContext& ctx) {
        int idx = throttleManager.getCurrentThrottleIndex();
        throttleManager.momentum().cycleMomentumLevel(idx);
        // Visual feedback will come from adding display indicator later (Stage 2)
    }
    
    void handleIncreaseThrottles(MenuContext& ctx) {
        throttleManager.changeNumberOfThrottles(true);
    }
    
    void handleDecreaseThrottles(MenuContext& ctx) {
        throttleManager.changeNumberOfThrottles(false);
    }
    
    void handleDisconnect(MenuContext& ctx) {
        if (systemStateManager.isOperating()) {
            locoManager.resetRestoreGuard();
            systemStateManager.setState(SystemState::WifiConnected);
            connectionManager.disconnect();
        }
    }
    
    void handleSleep(MenuContext& ctx) {
        deepSleepStart();
    }
    
    void handleDropBeforeAcquireToggle(MenuContext& ctx) {
        locoManager.toggleDropBeforeAcquire();
    }
    
    void handleLocoConfig(MenuContext& ctx) {
        inputManager.setMode(InputMode::LocoConfigWizard);
    }
    
    // List renderers (thin wrappers around existing functions)
    void renderRosterList() {
        // Switch to RosterSelection mode which will render the list via onEnter
        inputManager.setMode(InputMode::RosterSelection);
    }
    
    void renderFunctionList() {
        if (wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar()) == 0) return;
        // Switch to FunctionSelection mode which will render the list via onEnter
        inputManager.setMode(InputMode::FunctionSelection);
    }
    
    void renderTurnoutListThrow() {
        // Switch to TurnoutSelection mode which will render the list via onEnter
        inputManager.setMode(InputMode::TurnoutSelection);
    }
    
    void renderTurnoutListClose() {
        // Switch to TurnoutSelection mode which will render the list via onEnter
        inputManager.setMode(InputMode::TurnoutSelection);
    }
    
    void renderRouteList() {
        // Switch to RouteSelection mode which will render the list via onEnter
        inputManager.setMode(InputMode::RouteSelection);
    }
    
    void renderDropLocoList() {
        // Switch to DropLocoSelection mode which will render the list via onEnter
        inputManager.setMode(InputMode::DropLocoSelection);
    }
}

// ==================== Menu Definitions ====================

namespace MenuDefinitions {
    
    // Extras submenu (replicates items 10-19 from menuText array)
    const MenuItem extrasMenuItems[] = {
        MenuItem::action(1, "Fn Key Toggle", "N/A",
                        MenuHandlers::handleFunctionKeyToggle),
        
        MenuItem::action(2, "Edt Consist", "N/A",
                       MenuHandlers::handleEditConsist),
        
        MenuItem::action(3, "Momentum", "N/A",
                        MenuHandlers::handleMomentumToggle),
        
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
        
        MenuItem::action(0, "Loco Config", "N/A",
                        MenuHandlers::handleLocoConfig,
                        []() { return wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar()) > 0; })
    };
    const uint8_t extrasMenuSize = 10;
    
    // Main menu (replicates items 0-9 from menuText array)
    // Items 1-9 followed by item 0 (Function) to match keypad layout
    const MenuItem mainMenuItems[] = {
        MenuItem::input(1, "Add Loco", "* Cancel  # List or Enter ID",
                       MenuHandlers::handleAddLoco),
        
        MenuItem::list(2, "Drop Loco", "* Cancel  # All",
                      nullptr,  // Handler not used for LIST items
                      MenuHandlers::renderDropLocoList,
                      []() { return wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar()) > 0; }),
        
        MenuItem::action(3, "Toggle Dir", "N/A",
                        MenuHandlers::handleToggleDirection),
        
        MenuItem::action(4, "X SpeedStep", "N/A",
                        MenuHandlers::handleSpeedStep),
        
        MenuItem::input(5, "Throw Point", "* Cancel  # List or Enter ID",
                       MenuHandlers::handleThrowPoint),
        
        MenuItem::input(6, "Close Point", "* Cancel  # List or Enter ID",
                       MenuHandlers::handleClosePoint),
        
        MenuItem::input(7, "Route", "* Cancel  # List or Enter ID",
                       MenuHandlers::handleRoute),
        
        MenuItem::action(8, "Track Power", "N/A",
                        MenuHandlers::handleTrackPower),
        
        MenuItem::submenu(9, "Extras", "* Cancel  0-9 Select",
                         extrasMenuItems, extrasMenuSize),
        
        MenuItem::list(0, "Function", "* Cancel  0-9 Select",
                      MenuHandlers::handleFunction,
                      MenuHandlers::renderFunctionList,
                      []() { return wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar()) > 0; })
    };
    const uint8_t mainMenuSize = 10;
}
