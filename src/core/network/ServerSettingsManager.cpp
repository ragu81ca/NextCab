// ServerSettingsManager.cpp — Per-server settings ownership.
#include "ServerSettingsManager.h"
#include "WiThrottleConnectionManager.h"
#include "../ServerDataStore.h"
#include "../storage/ConfigStore.h"
#include "../../../static.h"

void ServerSettingsManager::begin(ConfigStore *configStore, ServerDataStore *dataStore,
                                  WiThrottleConnectionManager *connMgr) {
    configStore_ = configStore;
    dataStore_   = dataStore;
    connMgr_     = connMgr;
}

void ServerSettingsManager::setServerType(const String &type) {
    serverType_ = type;
    applyServerTypeDefaults();
}

void ServerSettingsManager::applyServerTypeDefaults() {
    if (serverType_.equals("DCC-EX")) applyDccExDefaults();
}

void ServerSettingsManager::applyDccExDefaults() {
    if (!dataStore_) return;
    dataStore_->applyDetectedTurnoutPrefix(DCC_EX_TURNOUT_PREFIX);
    dataStore_->applyDetectedRoutePrefix(DCC_EX_ROUTE_PREFIX);
}

void ServerSettingsManager::restoreForCurrentServer() {
    if (restoredForCurrentServer_) return;
    if (!configStore_ || !dataStore_ || !connMgr_) return;

    String name = connMgr_->selectedName();
    if (name.length() == 0) return;
    restoredForCurrentServer_ = true;

    ServerConfig cfg = configStore_->findServerConfig(name);
    if (cfg.turnoutPrefixSet) dataStore_->setTurnoutPrefix(cfg.turnoutPrefix);
    if (cfg.routePrefixSet)   dataStore_->setRoutePrefix(cfg.routePrefix);
}

void ServerSettingsManager::reset() {
    restoredForCurrentServer_ = false;
    serverType_ = "";
    if (dataStore_) dataStore_->clearPrefixConfiguration();
}

void ServerSettingsManager::savePrefixes(const String &turnoutPrefix, const String &routePrefix) {
    if (!dataStore_) return;
    dataStore_->setTurnoutPrefix(turnoutPrefix);
    dataStore_->setRoutePrefix(routePrefix);

    if (!configStore_ || !connMgr_) return;
    String name = connMgr_->selectedName();
    if (name.length() == 0) return;

    // Read-modify-write: saveServer replaces the stored entry wholesale, so the
    // existing record must be carried forward or acquired locos are lost.
    ServerConfig cfg = configStore_->findServerConfig(name);
    cfg.name             = name;
    cfg.host             = connMgr_->selectedIP().toString();
    cfg.port             = connMgr_->selectedPort();
    cfg.turnoutPrefix    = turnoutPrefix;
    cfg.routePrefix      = routePrefix;
    cfg.turnoutPrefixSet = true;
    cfg.routePrefixSet   = true;
    configStore_->saveServer(cfg);
}
