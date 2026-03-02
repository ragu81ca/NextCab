// ConfigStore.cpp — LittleFS + JSON persistent storage implementation.
#include "ConfigStore.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

static const char* WIFI_PATH    = "/config/wifi.json";
static const char* SERVERS_PATH = "/config/servers.json";

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

// ── Server configs (stub) ─────────────────────────────────────────────

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
        cfg.host           = srv["host"].as<String>();
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

        if (cfg.host.length() > 0) {
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
        if (srv.host == server.host && srv.port == server.port) {
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

ServerConfig ConfigStore::findServerConfig(const String& host, int port) {
    auto servers = loadServers();
    for (const auto& srv : servers) {
        if (srv.host == host && srv.port == port) return srv;
    }
    return {};  // empty defaults
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
