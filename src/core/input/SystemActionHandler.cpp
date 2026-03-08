#include "SystemActionHandler.h"

// Deep sleep remains in .ino (hardware-specific GPIO/wakeup config)
extern void deepSleepStart();

void SystemActionHandler::deepSleepStart() { ::deepSleepStart(); }