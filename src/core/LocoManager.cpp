// LocoManager.cpp — Loco acquisition, release, consist management, persistence
// Migrated from free functions in WiTcontroller.ino

#include "LocoManager.h"
#include "ThrottleManager.h"
#include "ServerDataStore.h"
#include "Renderer.h"
#include "UIState.h"
#include "storage/ConfigStore.h"
#include "network/WiThrottleConnectionManager.h"
#include "../../WiTcontroller.h"   // getMultiThrottleChar/Index, SHORT_DCC_ADDRESS_LIMIT, etc.
#include "../../static.h"          // DIRECTION_REVERSE_INDICATOR
#include "protocol/WiThrottleDelegate.h" // debug_print/debug_println macros
#include <algorithm>               // std::remove_if

// Static empty vector returned for out-of-range throttle indices
const std::vector<LocoConfig> LocoManager::emptyConfigs_;

void LocoManager::begin(WiThrottleProtocol *proto, ThrottleManager *throttle,
                         ServerDataStore *dataStore, Renderer *renderer, UIState *uiState,
                         ConfigStore *configStore, WiThrottleConnectionManager *connMgr) {
    proto_       = proto;
    throttle_    = throttle;
    dataStore_   = dataStore;
    renderer_    = renderer;
    uiState_     = uiState;
    configStore_ = configStore;
    connMgr_     = connMgr;

    // Self-register sound config cache as a loco-changed listener
    onLocoChanged([this](const LocoChangeEvent &e) { handleLocoChanged(e); });
}

// ── Acquire / Release ───────────────────────────────────────────────────
void LocoManager::acquireLoco(int throttleIndex, const String &loco) {
    debug_print("acquireLoco(): T"); debug_print(throttleIndex);
    debug_print(" loco: "); debug_println(loco);

    char tChar = getMultiThrottleChar(throttleIndex);

    if (dropBeforeAcquire_ && proto_->getNumberOfLocomotives(tChar) > 0) {
        proto_->releaseLocomotive(tChar, "*");
    }

    proto_->addLocomotive(tChar, loco);
    proto_->getDirection(tChar, loco);
    proto_->getSpeed(tChar);
    throttle_->resetFunctionStates(throttleIndex);

    notifyLocoChanged({throttleIndex, LocoChangeType::Acquired, loco});
}
void LocoManager::releaseAllLocos(int throttleIndex) {
    char tChar = getMultiThrottleChar(throttleIndex);
    String loco;
    int numLocos = proto_->getNumberOfLocomotives(tChar);
    // Collect addresses before releasing (protocol removes them as we go)
    std::vector<String> released;
    released.reserve(numLocos);
    for (int index = numLocos - 1; index >= 0; index--) {
        released.push_back(proto_->getLocomotiveAtPosition(tChar, index));
    }
    if (numLocos > 0) {
        for (auto &addr : released) {
            proto_->releaseLocomotive(tChar, addr);
            renderer_->renderSpeed();
        }
        throttle_->resetFunctionLabels(throttleIndex);
        uiState_->functionPage = 0;
    }
    for (auto &addr : released) {
        notifyLocoChanged({throttleIndex, LocoChangeType::Released, addr});
    }
}

void LocoManager::releaseOneLoco(int throttleIndex, const String &loco) {
    debug_print("releaseOneLoco(): "); debug_print(throttleIndex); debug_print(": "); debug_println(loco);
    char tChar = getMultiThrottleChar(throttleIndex);
    proto_->releaseLocomotive(tChar, loco);
    throttle_->resetFunctionLabels(throttleIndex);
    uiState_->functionPage = 0;
    notifyLocoChanged({throttleIndex, LocoChangeType::Released, loco});
    debug_println("releaseOneLoco(): end");
}

void LocoManager::releaseOneLocoByIndex(int throttleIndex, int index) {
    debug_print("releaseOneLocoByIndex(): "); debug_print(throttleIndex); debug_print(": "); debug_println(index);
    char tChar = getMultiThrottleChar(throttleIndex);
    String loco;
    if (index <= proto_->getNumberOfLocomotives(tChar)) {
        loco = proto_->getLocomotiveAtPosition(tChar, index);
        proto_->releaseLocomotive(tChar, loco);
        throttle_->resetFunctionLabels(throttleIndex);
        uiState_->functionPage = 0;
    }
    notifyLocoChanged({throttleIndex, LocoChangeType::Released, loco});
    debug_println("releaseOneLocoByIndex(): end");
}

// ── Steal ───────────────────────────────────────────────────────────────

void LocoManager::stealLoco(int throttleIndex, const String &address) {
    proto_->stealLocomotive(throttleIndex, address);
    notifyLocoChanged({throttleIndex, LocoChangeType::Acquired, address});
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

// ── Loco persistence ────────────────────────────────────────────────────

void LocoManager::resetRestoreGuard() {
    locosRestoredForCurrentServer_ = false;
}

void LocoManager::restoreLocos() {
    if (!configStore_ || !connMgr_) return;

#if RESTORE_ACQUIRED_LOCOS
    if (locosRestoredForCurrentServer_) return;

    String name = connMgr_->selectedName();
    if (name.length() == 0) return;

    locosRestoredForCurrentServer_ = true;

    ServerConfig cfg = configStore_->findServerConfig(name);

    // Apply turnout/route prefixes if stored (DCC-EX heuristic may override later)
    if (cfg.turnoutPrefix.length() > 0) dataStore_->setTurnoutPrefix(cfg.turnoutPrefix);
    if (cfg.routePrefix.length() > 0)   dataStore_->setRoutePrefix(cfg.routePrefix);

    // Restore locos
    for (int t = 0; t < (int)cfg.throttleLocos.size() && t < throttle_->getMaxThrottles(); t++) {
        char throttleChar = '0' + t;
        for (const auto& addr : cfg.throttleLocos[t]) {
            if (addr.length() > 0) {
                proto_->addLocomotive(throttleChar, addr);
                proto_->getDirection(throttleChar, addr);
                proto_->getSpeed(throttleChar);
            }
        }
    }

    Serial.printf("[Locos] Restored %d throttle(s) of locos for '%s'\n",
                  (int)cfg.throttleLocos.size(), name.c_str());

    // Notify listeners for all restored throttles
    for (int t = 0; t < (int)cfg.throttleLocos.size() && t < throttle_->getMaxThrottles(); t++) {
        for (const auto& addr : cfg.throttleLocos[t]) {
            if (addr.length() > 0) {
                notifyLocoChanged({t, LocoChangeType::Acquired, addr});
            }
        }
    }
#endif
}

void LocoManager::saveLocos() {
    if (!configStore_ || !connMgr_) return;

    String name = connMgr_->selectedName();
    if (name.length() == 0) return;

    ServerConfig cfg;
    cfg.name           = name;
    cfg.host           = connMgr_->selectedIP().toString();
    cfg.port           = connMgr_->selectedPort();
    cfg.turnoutPrefix  = dataStore_->turnoutPrefix();
    cfg.routePrefix    = dataStore_->routePrefix();

    int maxT = throttle_->getMaxThrottles();
    cfg.throttleLocos.resize(maxT);
    for (int t = 0; t < maxT; t++) {
        char throttleChar = '0' + t;
        int numLocos = proto_->getNumberOfLocomotives(throttleChar);
        for (int j = 0; j < numLocos; j++) {
            String loco = proto_->getLocomotiveAtPosition(throttleChar, j);
            if (loco.length() > 0) {
                cfg.throttleLocos[t].push_back(loco);
            }
        }
    }

    configStore_->saveServer(cfg);
    Serial.printf("[Locos] Saved locos for '%s'\n", name.c_str());
}

// ── Loco-changed observer ───────────────────────────────────────────────────

void LocoManager::onLocoChanged(LocoChangeCallback cb) {
    locoChangeCallbacks_.push_back(std::move(cb));
}

void LocoManager::notifyLocoChanged(const LocoChangeEvent &event) {
    for (auto &cb : locoChangeCallbacks_) {
        cb(event);
    }
}

// ── Sound throttle config cache ───────────────────────────────────────────

void LocoManager::handleLocoChanged(const LocoChangeEvent &event) {
    int t = event.throttleIndex;
    if (t < 0 || t >= WIT_MAX_THROTTLES) return;

    if (event.type == LocoChangeType::Acquired) {
        if (!configStore_ || event.address.length() == 0) return;
        LocoConfig cfg = configStore_->loadLocoConfig(event.address);
        if (cfg.soundThrottle) {
            soundConfigs_[t].push_back(cfg);
            Serial.printf("[Locos] T%d: sound throttle enabled for %s\n",
                          t, event.address.c_str());
        }
    } else {  // Released
        auto &vec = soundConfigs_[t];
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [&](const LocoConfig &c) { return c.address == event.address; }),
            vec.end());
    }
}

const std::vector<LocoConfig>& LocoManager::soundConfigs(int throttleIndex) const {
    if (throttleIndex < 0 || throttleIndex >= WIT_MAX_THROTTLES) return emptyConfigs_;
    return soundConfigs_[throttleIndex];
}

bool LocoManager::hasSoundThrottle(int throttleIndex) const {
    if (throttleIndex < 0 || throttleIndex >= WIT_MAX_THROTTLES) return false;
    return !soundConfigs_[throttleIndex].empty();
}
