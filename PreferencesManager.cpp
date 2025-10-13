// PreferencesManager.cpp (moved to root for Arduino build system recognition)
#include "core/PreferencesManager.h"
#include "static.h"

PreferencesManager::PreferencesManager() : nvsInit(false), preferencesRead(false) {}

void PreferencesManager::open(const char *ns, bool readOnly) {
    prefs.begin(ns, readOnly);
}
void PreferencesManager::close() { prefs.end(); }

void PreferencesManager::begin(bool forceClear) {
    if (preferencesRead && !forceClear) return;
    open("WitController", true);
    nvsInit = prefs.isKey("nvsInit");
    close();
    if (!nvsInit || forceClear) {
        open("WitController", false);
        prefs.putBool("nvsInit", true);
        close();
        nvsInit = true;
    }
}

void PreferencesManager::restoreLocos(WiThrottleProtocol &proto, char throttleIndexCharStart) {
    if (preferencesRead) return;
    if (!RESTORE_ACQUIRED_LOCOS) { preferencesRead = true; return; }
    open("WitController", true);
    nvsInit = prefs.isKey("nvsInit");
    if (!nvsInit) { close(); return; }

    for (int i=0; i<MAX_THROTTLES; i++) {
        char throttleChar = throttleIndexCharStart + i;
        for (int j=0; j<10; j++) {
            char key[4]; key[0]='L'; key[1]=throttleChar; key[2]='0'+j; key[3]='\0';
            if (prefs.isKey(key)) {
                String locoNum = prefs.getString(key);
                int locoNo = locoNum.toInt();
                String locoWithLength;
                if ( (locoNo > SHORT_DCC_ADDRESS_LIMIT) || ((locoNo <= SHORT_DCC_ADDRESS_LIMIT) && (locoNum.charAt(0)=='0')) ) {
                    locoWithLength = "L" + locoNum;
                } else {
                    locoWithLength = "S" + locoNum;
                }
                proto.addLocomotive(throttleChar, locoWithLength);
                proto.getDirection(throttleChar, locoWithLength);
                proto.getSpeed(throttleChar);
            }
        }
    }
    preferencesRead = true;
    close();
}

void PreferencesManager::saveLocos(WiThrottleProtocol &proto, int maxThrottles) {
    if (!nvsInit) return;
    open("WitController", false);
    prefs.putBool("nvsInit", true);
    for (int i=0; i<maxThrottles; i++) {
        char throttleChar = '0' + i;
        for (int j=0; j<10; j++) {
            char key[4]; key[0]='L'; key[1]=throttleChar; key[2]='0'+j; key[3]='\0';
            if (j < proto.getNumberOfLocomotives(throttleChar)) {
                String loco = proto.getLocomotiveAtPosition(throttleChar, j);
                String locoNumber = loco.substring(1);
                prefs.putString(key, locoNumber);
            } else {
                if (prefs.isKey(key)) { prefs.remove(key); }
            }
        }
    }
    close();
}

void PreferencesManager::clear() { begin(true); }
