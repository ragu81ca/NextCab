// PreferencesManager.h - relocated and cleaned
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <WiThrottleProtocol.h>

class PreferencesManager {
public:
	PreferencesManager();
	void begin(bool forceClear=false);
	void restoreLocos(WiThrottleProtocol &proto, char throttleIndexCharStart='0');
	void saveLocos(WiThrottleProtocol &proto, int maxThrottles);
	void clear();
	bool isInitialized() const { return nvsInit; }
	bool locosRestored() const { return preferencesRead; }
private:
	void open(const char *ns, bool readOnly);
	void close();
	Preferences prefs;
	bool nvsInit;
	bool preferencesRead;
};
