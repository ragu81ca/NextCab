// OledRenderer.h - Centralized OLED rendering abstraction
#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "static.h"          // constants & macros
#include "WiTcontroller.h"    // extern globals & function prototypes
#include <WiThrottleProtocol.h> // for TurnoutAction and protocol access

class OledRenderer {
public:
    explicit OledRenderer(U8G2 &disp) : display(disp) {}

    // Direct rendering methods (no recursion through legacy free functions)
    void renderSpeed();
    void renderSpeedStepMultiplier();
    void renderBattery();
    void renderArray(bool a, bool b) { renderArrayInternal(a,b,true,false); }
    void renderArray(bool a, bool b, bool c) { renderArrayInternal(a,b,c,false); }
    void renderArray(bool a, bool b, bool c, bool d) { renderArrayInternal(a,b,c,d); }
    void clearArray();
    void renderDirectCommands();
    void setAppName() { setAppnameForOled(); }

    // Migrated complex renderers (Phase 2)
    void renderFoundSsids(const String &soFar);
    void renderRoster(const String &soFar);
    void renderTurnoutList(const String &soFar, TurnoutAction action);
    void renderRouteList(const String &soFar);
    void renderFunctionList(const String &soFar);
    void renderEnterPassword();
    void renderFunctions();
    void renderEditConsist();
    void renderHeartbeatCheck();
    void renderAllLocos(bool hideLeadLoco);
    void renderMenu(const String &soFar, bool primeMenu);

private:
    U8G2 &display;
    void renderArrayInternal(bool isThreeColumns, bool isPassword, bool sendBuffer, bool drawTopLine);
};

extern OledRenderer oledRenderer; // instance defined in OledRenderer.cpp
