// ConfigStore.cpp — LittleFS + JSON persistent storage implementation.
#include "ConfigStore.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

static const char* DEVICE_PATH  = "/config/device.json";
static const char* WIFI_PATH    = "/config/wifi.json";
static const char* SERVERS_PATH = "/config/servers.json";
static const char* LOCOS_DIR    = "/config/locos";

// ── Lifecycle ──────────────────────────────────────────────────────────

void ConfigStore::begin() {
    if (mounted_) return;

    // true = format flash on first use (partition not yet initialised)
    if (!LittleFS.begin(true)) {
        Serial.println("[ConfigStore] ERROR: LittleFS mount FAILED even after format!");
        Serial.println("[ConfigStore] Check that the partition table has a 'spiffs' type partition.");
        return;
    }
    mounted_ = true;
    Serial.println("[ConfigStore] LittleFS mounted OK");
    ensureDir("/config");
    ensureDir(LOCOS_DIR);

    // Verify we can actually write and read back
    auto networks = loadWifiNetworks();
    Serial.printf("[ConfigStore] %d WiFi network(s) in storage at boot\n", networks.size());
}

bool ConfigStore::factoryReset() {
    Serial.println("[ConfigStore] Factory reset — formatting LittleFS");
    LittleFS.format();
    mounted_ = false;
    begin();  // remount
    return mounted_;
}

// ── WiFi credentials ──────────────────────────────────────────────────

std::vector<WifiCredential> ConfigStore::loadWifiNetworks() {
    std::vector<WifiCredential> result;
    if (!mounted_) return result;

    String content = readFile(WIFI_PATH);
    if (content.isEmpty()) return result;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, content);
    if (err) {
        Serial.printf("[ConfigStore] wifi.json parse error: %s\n", err.c_str());
        return result;
    }

    JsonArray networks = doc["networks"].as<JsonArray>();
    if (networks.isNull()) return result;

    for (JsonObject net : networks) {
        WifiCredential cred;
        cred.ssid     = net["ssid"].as<String>();
        cred.password = net["password"].as<String>();
        if (cred.ssid.length() > 0) {
            result.push_back(cred);
        }
    }

    Serial.printf("[ConfigStore] Loaded %d WiFi network(s) from storage\n", result.size());
    return result;
}

void ConfigStore::saveWifiNetwork(const String& ssid, const String& password) {
    if (!mounted_) return;

    // Load existing networks, update or append
    auto networks = loadWifiNetworks();

    bool found = false;
    for (auto& net : networks) {
        if (net.ssid == ssid) {
            net.password = password;
            found = true;
            Serial.printf("[ConfigStore] Updated password for SSID: %s\n", ssid.c_str());
            break;
        }
    }
    if (!found) {
        networks.push_back({ssid, password});
        Serial.printf("[ConfigStore] Added new SSID: %s\n", ssid.c_str());
    }

    // Serialize back
    JsonDocument doc;
    JsonArray arr = doc["networks"].to<JsonArray>();
    for (const auto& net : networks) {
        JsonObject obj = arr.add<JsonObject>();
        obj["ssid"]     = net.ssid;
        obj["password"] = net.password;
    }

    String output;
    serializeJsonPretty(doc, output);
    writeFile(WIFI_PATH, output);
}

void ConfigStore::removeWifiNetwork(const String& ssid) {
    if (!mounted_) return;

    auto networks = loadWifiNetworks();
    bool removed = false;
    for (auto it = networks.begin(); it != networks.end(); ++it) {
        if (it->ssid == ssid) {
            networks.erase(it);
            removed = true;
            Serial.printf("[ConfigStore] Removed SSID: %s\n", ssid.c_str());
            break;
        }
    }
    if (!removed) return;

    JsonDocument doc;
    JsonArray arr = doc["networks"].to<JsonArray>();
    for (const auto& net : networks) {
        JsonObject obj = arr.add<JsonObject>();
        obj["ssid"]     = net.ssid;
        obj["password"] = net.password;
    }

    String output;
    serializeJsonPretty(doc, output);
    writeFile(WIFI_PATH, output);
}

String ConfigStore::findPasswordForSsid(const String& ssid) {
    auto networks = loadWifiNetworks();
    for (const auto& net : networks) {
        if (net.ssid == ssid) return net.password;
    }
    return "";
}

// ── Device config (Tier 1) ─────────────────────────────────────────────

DeviceConfig ConfigStore::loadDeviceConfig() {
    DeviceConfig cfg;  // struct defaults = compile-time constants
    if (!mounted_) return cfg;

    String content = readFile(DEVICE_PATH);
    if (content.isEmpty()) return cfg;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, content);
    if (err) {
        Serial.printf("[ConfigStore] device.json parse error: %s\n", err.c_str());
        return cfg;
    }

    // Only override fields that are actually present in the JSON.
    if (doc["speedStep"].is<int>())              cfg.speedStep             = doc["speedStep"];
    if (doc["speedStepMultiplier"].is<int>())     cfg.speedStepMultiplier    = doc["speedStepMultiplier"];
    if (doc["displaySpeedAsPercent"].is<bool>())   cfg.displaySpeedAsPercent  = doc["displaySpeedAsPercent"];
    if (doc["encoderCwIsIncrease"].is<bool>())     cfg.encoderCwIsIncrease    = doc["encoderCwIsIncrease"];
    if (doc["heartbeatEnabled"].is<bool>())        cfg.heartbeatEnabled       = doc["heartbeatEnabled"];
    if (doc["restoreAcquiredLocos"].is<bool>())    cfg.restoreAcquiredLocos   = doc["restoreAcquiredLocos"];

    Serial.println("[ConfigStore] Loaded device config");
    return cfg;
}

void ConfigStore::saveDeviceConfig(const DeviceConfig& cfg) {
    if (!mounted_) return;

    JsonDocument doc;
    doc["speedStep"]             = cfg.speedStep;
    doc["speedStepMultiplier"]    = cfg.speedStepMultiplier;
    doc["displaySpeedAsPercent"]  = cfg.displaySpeedAsPercent;
    doc["encoderCwIsIncrease"]    = cfg.encoderCwIsIncrease;
    doc["heartbeatEnabled"]       = cfg.heartbeatEnabled;
    doc["restoreAcquiredLocos"]   = cfg.restoreAcquiredLocos;

    String output;
    serializeJsonPretty(doc, output);
    writeFile(DEVICE_PATH, output);
}

// ── Server configs (Tier 2) ────────────────────────────────────────────

std::vector<ServerConfig> ConfigStore::loadServers() {
    std::vector<ServerConfig> result;
    if (!mounted_) return result;

    String content = readFile(SERVERS_PATH);
    if (content.isEmpty()) return result;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, content);
    if (err) {
        Serial.printf("[ConfigStore] servers.json parse error: %s\n", err.c_str());
        return result;
    }

    JsonArray servers = doc["servers"].as<JsonArray>();
    if (servers.isNull()) return result;

    for (JsonObject srv : servers) {
        ServerConfig cfg;
        cfg.name           = srv["name"].as<String>();
        cfg.host           = srv["host"] | "";
        cfg.port           = srv["port"] | 0;
        cfg.turnoutPrefix  = srv["turnoutPrefix"] | "";
        cfg.routePrefix    = srv["routePrefix"]   | "";

        // Deserialize per-throttle loco lists
        JsonArray locos = srv["locos"].as<JsonArray>();
        if (!locos.isNull()) {
            for (JsonArray throttle : locos) {
                std::vector<String> addrs;
                for (JsonVariant addr : throttle) {
                    String a = addr.as<String>();
                    if (a.length() > 0) addrs.push_back(a);
                }
                cfg.throttleLocos.push_back(addrs);
            }
        }

        if (cfg.name.length() > 0) {
            result.push_back(cfg);
        }
    }

    Serial.printf("[ConfigStore] Loaded %d server(s) from storage\n", result.size());
    return result;
}

void ConfigStore::saveServer(const ServerConfig& server) {
    if (!mounted_) return;

    auto servers = loadServers();

    bool found = false;
    for (auto& srv : servers) {
        if (srv.name == server.name) {
            srv.host           = server.host;
            srv.port           = server.port;
            srv.turnoutPrefix  = server.turnoutPrefix;
            srv.routePrefix    = server.routePrefix;
            srv.throttleLocos  = server.throttleLocos;
            found = true;
            break;
        }
    }
    if (!found) {
        servers.push_back(server);
    }

    JsonDocument doc;
    JsonArray arr = doc["servers"].to<JsonArray>();
    for (const auto& srv : servers) {
        JsonObject obj = arr.add<JsonObject>();
        obj["name"]           = srv.name;
        obj["host"]           = srv.host;
        obj["port"]           = srv.port;
        obj["turnoutPrefix"]  = srv.turnoutPrefix;
        obj["routePrefix"]    = srv.routePrefix;

        // Serialize per-throttle loco lists
        JsonArray locosArr = obj["locos"].to<JsonArray>();
        for (const auto& throttle : srv.throttleLocos) {
            JsonArray throttleArr = locosArr.add<JsonArray>();
            for (const auto& addr : throttle) {
                throttleArr.add(addr);
            }
        }
    }

    String output;
    serializeJsonPretty(doc, output);
    writeFile(SERVERS_PATH, output);
}

ServerConfig ConfigStore::findServerConfig(const String& name) {
    auto servers = loadServers();
    for (const auto& srv : servers) {
        if (srv.name == name) return srv;
    }
    return {};  // empty defaults
}

// ── Loco configs (Tier 3) ─────────────────────────────────────────────
// Each loco gets its own file: /config/locos/<address>.json
// This keeps memory usage O(1) — only one tiny file (~80 bytes) is
// parsed at a time, regardless of how many locos are configured.

static String locoPath(const String& address) {
    return String(LOCOS_DIR) + "/" + address + ".json";
}

static LocoConfig parseLocoFile(const String& address, const String& content) {
    LocoConfig cfg;
    cfg.address = address;
    if (content.isEmpty()) return cfg;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, content);
    if (err) {
        Serial.printf("[ConfigStore] %s.json parse error: %s\n", address.c_str(), err.c_str());
        return cfg;
    }

    cfg.soundThrottle    = doc["soundThrottle"]    | false;
    cfg.funcThrottleUp   = doc["funcThrottleUp"]   | -1;
    cfg.funcThrottleDown = doc["funcThrottleDown"] | -1;
    cfg.funcBrake        = doc["funcBrake"]        | -1;
    return cfg;
}

LocoConfig ConfigStore::loadLocoConfig(const String& address) {
    LocoConfig cfg;
    cfg.address = address;
    if (!mounted_ || address.isEmpty()) return cfg;

    String content = readFile(locoPath(address));
    return parseLocoFile(address, content);
}

void ConfigStore::saveLocoConfig(const LocoConfig& cfg) {
    if (!mounted_ || cfg.address.isEmpty()) return;

    JsonDocument doc;
    doc["soundThrottle"]    = cfg.soundThrottle;
    doc["funcThrottleUp"]   = cfg.funcThrottleUp;
    doc["funcThrottleDown"] = cfg.funcThrottleDown;
    doc["funcBrake"]        = cfg.funcBrake;

    String output;
    serializeJsonPretty(doc, output);
    writeFile(locoPath(cfg.address), output);
}

std::vector<LocoConfig> ConfigStore::loadAllLocoConfigs() {
    std::vector<LocoConfig> result;
    if (!mounted_) return result;

    File dir = LittleFS.open(LOCOS_DIR);
    if (!dir || !dir.isDirectory()) return result;

    File entry;
    while ((entry = dir.openNextFile())) {
        String name = entry.name();  // e.g. "L1234.json"
        if (!name.endsWith(".json")) { entry.close(); continue; }

        String content = entry.readString();
        entry.close();

        // Strip .json suffix to get the address
        String address = name.substring(0, name.length() - 5);
        // On some ESP32 cores, entry.name() includes the full path
        int lastSlash = address.lastIndexOf('/');
        if (lastSlash >= 0) address = address.substring(lastSlash + 1);

        LocoConfig cfg = parseLocoFile(address, content);
        result.push_back(cfg);
    }
    dir.close();

    Serial.printf("[ConfigStore] Loaded %d loco config(s)\n", result.size());
    return result;
}

// ── File I/O helpers ──────────────────────────────────────────────────

String ConfigStore::readFile(const String& path) {
    File f = LittleFS.open(path, "r");
    if (!f) return "";
    String content = f.readString();
    f.close();
    return content;
}

bool ConfigStore::writeFile(const String& path, const String& content) {
    File f = LittleFS.open(path, "w");
    if (!f) {
        Serial.printf("[ConfigStore] Failed to open %s for writing\n", path.c_str());
        return false;
    }
    f.print(content);
    f.close();
    Serial.printf("[ConfigStore] Wrote %s (%d bytes)\n", path.c_str(), content.length());
    return true;
}

bool ConfigStore::ensureDir(const String& path) {
    if (LittleFS.exists(path)) return true;
    return LittleFS.mkdir(path);
}
