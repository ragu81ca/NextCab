// OledRenderer.h - relocated to src/core
#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <WiThrottleProtocol.h> // for TurnoutAction

class OledRenderer {
public:
    explicit OledRenderer(U8G2 &display);

    // Public rendering entry points used by application code
    void renderFoundSsids(const String &soFar);
    void renderRoster(const String &soFar);
    void renderTurnoutList(const String &soFar, TurnoutAction action);
    void renderRouteList(const String &soFar);
    void renderFunctionList(const String &soFar);
    void renderEnterPassword();
    void renderEditConsist();
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

private:
    void renderArrayInternal(bool isThreeColumns, bool isPassword, bool sendBuffer, bool drawTopLine);
    void renderFunctions();
    void renderAllLocos(bool hideLeadLoco); // now private again
    
    // Helper functions for unified loco list rendering
    bool checkNeedSuffixes(char throttleChar, int numLocos);
    String formatLocoDisplay(const String &loco, bool needSuffixes);
    // (kept declaration moved to public)

    U8G2 &display;
};
