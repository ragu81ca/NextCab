// LocoManager.cpp — Loco acquisition, release, consist management
// Migrated from free functions in WiTcontroller.ino

#include "LocoManager.h"
#include "ThrottleManager.h"
#include "ServerDataStore.h"
#include "Renderer.h"
#include "UIState.h"
#include "../../WiTcontroller.h"   // getMultiThrottleChar/Index, SHORT_DCC_ADDRESS_LIMIT, etc.
#include "../../static.h"          // DIRECTION_REVERSE_INDICATOR
#include "protocol/WiThrottleDelegate.h" // debug_print/debug_println macros

void LocoManager::begin(WiThrottleProtocol *proto, ThrottleManager *throttle,
                         ServerDataStore *dataStore, Renderer *renderer, UIState *uiState) {
    proto_    = proto;
    throttle_ = throttle;
    dataStore_ = dataStore;
    renderer_ = renderer;
    uiState_  = uiState;
}

// ── Acquire / Release ───────────────────────────────────────────────────

void LocoManager::releaseAllLocos(int throttleIndex) {
    char tChar = getMultiThrottleChar(throttleIndex);
    String loco;
    if (proto_->getNumberOfLocomotives(tChar) > 0) {
        for (int index = proto_->getNumberOfLocomotives(tChar) - 1; index >= 0; index--) {
            loco = proto_->getLocomotiveAtPosition(tChar, index);
            proto_->releaseLocomotive(tChar, loco);
            renderer_->renderSpeed();  // note the released locos may not be visible
        }
        throttle_->resetFunctionLabels(throttleIndex);
        uiState_->functionPage = 0;
    }
}

void LocoManager::releaseOneLoco(int throttleIndex, const String &loco) {
    debug_print("releaseOneLoco(): "); debug_print(throttleIndex); debug_print(": "); debug_println(loco);
    char tChar = getMultiThrottleChar(throttleIndex);
    proto_->releaseLocomotive(tChar, loco);
    throttle_->resetFunctionLabels(throttleIndex);
    uiState_->functionPage = 0;
    debug_println("releaseOneLoco(): end");
}

void LocoManager::releaseOneLocoByIndex(int throttleIndex, int index) {
    debug_print("releaseOneLocoByIndex(): "); debug_print(throttleIndex); debug_print(": "); debug_println(index);
    char tChar = getMultiThrottleChar(throttleIndex);
    if (index <= proto_->getNumberOfLocomotives(tChar)) {
        String loco = proto_->getLocomotiveAtPosition(tChar, index);
        proto_->releaseLocomotive(tChar, loco);
        throttle_->resetFunctionLabels(throttleIndex);
        uiState_->functionPage = 0;
    }
    debug_println("releaseOneLocoByIndex(): end");
}

// ── Steal ───────────────────────────────────────────────────────────────

void LocoManager::stealLoco(int throttleIndex, const String &address) {
    proto_->stealLocomotive(throttleIndex, address);
}

void LocoManager::stealLoco(char multiThrottle, const String &address) {
    int idx = getMultiThrottleIndex(multiThrottle);
    stealLoco(idx, address);
}

// ── Consist facing ──────────────────────────────────────────────────────

void LocoManager::toggleLocoFacing(int throttleIndex, const String &loco) {
    debug_println("toggleLocoFacing()");
    char tChar = getMultiThrottleChar(throttleIndex);
    debug_print("toggleLocoFacing(): "); debug_println(loco);
    for (int i = 0; i < proto_->getNumberOfLocomotives(tChar); i++) {
        if (proto_->getLocomotiveAtPosition(tChar, i).equals(loco)) {
            Direction currentDirection = proto_->getDirection(tChar, loco);
            debug_print("toggleLocoFacing(): loco: ");  debug_print(loco);
            debug_print(" current direction: "); debug_println(currentDirection);
            if (currentDirection == Forward) {
                proto_->setDirection(tChar, loco, Reverse, true);
            } else {
                proto_->setDirection(tChar, loco, Forward, true);
            }
            break;
        }
    }
}

// ── Display helpers ─────────────────────────────────────────────────────

String LocoManager::getDisplayLocoString(int throttleIndex, int index) {
    char tChar = getMultiThrottleChar(throttleIndex);
    String loco = proto_->getLocomotiveAtPosition(tChar, index);
    String locoNumber = loco.substring(1);

    #ifdef DISPLAY_LOCO_NAME
    {
        String rName = dataStore_->findRosterNameByAddress(locoNumber.toInt());
        if (rName.length() > 0) locoNumber = rName;
    }
    #endif

    if (!proto_->getLocomotiveAtPosition(tChar, 0).equals(loco)) { // not the lead loco
        Direction leadLocoDirection
            = proto_->getDirection(tChar, proto_->getLocomotiveAtPosition(tChar, 0));

        for (int i = 0; i < proto_->getNumberOfLocomotives(tChar); i++) {
            if (proto_->getLocomotiveAtPosition(tChar, i).equals(loco)) {
                if (proto_->getDirection(tChar, loco) != leadLocoDirection) {
                    locoNumber = locoNumber + DIRECTION_REVERSE_INDICATOR;
                }
                break;
            }
        }
    }
    return locoNumber;
}

String LocoManager::getLocoWithLength(const String &loco) {
    int locoNo = loco.toInt();
    String locoWithLength = "";
    if ((locoNo > SHORT_DCC_ADDRESS_LIMIT)
        || ((locoNo <= SHORT_DCC_ADDRESS_LIMIT) && (loco.charAt(0) == '0')
            && (!serverType_.equals("DCC-EX")))) {
        locoWithLength = "L" + loco;
    } else {
        locoWithLength = "S" + loco;
    }
    return locoWithLength;
}

// ── Drop-before-acquire toggle ──────────────────────────────────────────

void LocoManager::toggleDropBeforeAcquire() {
    dropBeforeAcquire_ = !dropBeforeAcquire_;
    debug_print("Drop Before Acquire: ");
    debug_println(dropBeforeAcquire_ ? "Enabled" : "Disabled");
}
