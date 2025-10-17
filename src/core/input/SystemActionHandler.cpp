#include "SystemActionHandler.h"
#include "../../../WiTcontroller.h" // for extern functions & TrackPower enum

// Forward externs from sketch
extern void powerOnOff(TrackPower p);
extern void powerToggle(void);
extern void batteryShowToggle(void);
extern void deepSleepStart();

void SystemActionHandler::powerOnOff(TrackPower p) { ::powerOnOff(p); }
void SystemActionHandler::powerToggle() { ::powerToggle(); }
void SystemActionHandler::batteryShowToggle() { ::batteryShowToggle(); }
void SystemActionHandler::deepSleepStart() { ::deepSleepStart(); }