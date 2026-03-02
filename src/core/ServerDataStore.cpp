// ServerDataStore.cpp — Implementation
#include "ServerDataStore.h"

const String ServerDataStore::empty_;

// ── Roster ──────────────────────────────────────────────────────────────────

void ServerDataStore::setRosterSize(int size) {
    rosterSize_ = size;
}

void ServerDataStore::addRosterEntry(int index, const String &name, int address, char length) {
    if (index < 0 || index >= MAX_ROSTER || index >= rosterSize_) return;
    rosterIndex_[index]      = index;
    rosterSortedIndex_[index] = index;
    rosterName_[index]       = name;
    rosterAddress_[index]    = address;
    rosterLength_[index]     = length;

#if ROSTER_SORT_SEQUENCE == 1
    strncpy(rosterSortStrings_[index],
            ((name + "          ").substring(0, 10) + ":" +
             (index < 10 ? "0" : "") + String(index)).c_str(), 13);
    rosterSortPointers_[index] = rosterSortStrings_[index];
#elif ROSTER_SORT_SEQUENCE == 2
    char buf[11];
    sprintf(buf, "%10d", address);
    strncpy(rosterSortStrings_[index],
            (String(buf) + ":" + (index < 10 ? "0" : "") + String(index)).c_str(), 13);
    rosterSortPointers_[index] = rosterSortStrings_[index];
#endif

    // When the last entry arrives, sort and build sorted-index mapping
    if (index == rosterSize_ - 1) {
        sortRoster();
    }
}

void ServerDataStore::sortRoster() {
#if ROSTER_SORT_SEQUENCE > 0
    qsort(rosterSortPointers_, rosterSize_, sizeof rosterSortPointers_[0], compareStrings);
    for (int i = 0; i < rosterSize_; i++) {
        rosterSortedIndex_[i] =
            (rosterSortPointers_[i][11] - '0') * 10 +
            (rosterSortPointers_[i][12] - '0');
    }
#endif
}

int ServerDataStore::rosterSortedIndex(int i) const {
    if (i < 0 || i >= rosterSize_) return 0;
    return rosterSortedIndex_[i];
}

const String &ServerDataStore::rosterName(int i) const {
    if (i < 0 || i >= MAX_ROSTER) return empty_;
    return rosterName_[i];
}

int ServerDataStore::rosterAddress(int i) const {
    if (i < 0 || i >= MAX_ROSTER) return 0;
    return rosterAddress_[i];
}

char ServerDataStore::rosterLength(int i) const {
    if (i < 0 || i >= MAX_ROSTER) return 'S';
    return rosterLength_[i];
}

String ServerDataStore::findRosterNameByAddress(int address) const {
    for (int i = 0; i < MAX_ROSTER; i++) {
        if (rosterAddress_[i] == address && rosterName_[i].length() > 0) {
            return rosterName_[i];
        }
    }
    return "";
}

// ── Turnouts ────────────────────────────────────────────────────────────────

void ServerDataStore::setTurnoutListSize(int size) {
    turnoutListSize_ = size;
}

void ServerDataStore::addTurnoutEntry(int index, const String &sysName, const String &userName, int state) {
    if (index < 0 || index >= MAX_TURNOUT_LIST || index >= turnoutListSize_) return;
    turnoutListIndex_[index]    = index;
    turnoutListSysName_[index]  = sysName;
    turnoutListUserName_[index] = userName;
    turnoutListState_[index]    = state;
}

const String &ServerDataStore::turnoutSysName(int i) const {
    if (i < 0 || i >= MAX_TURNOUT_LIST) return empty_;
    return turnoutListSysName_[i];
}

const String &ServerDataStore::turnoutUserName(int i) const {
    if (i < 0 || i >= MAX_TURNOUT_LIST) return empty_;
    return turnoutListUserName_[i];
}

int ServerDataStore::turnoutState(int i) const {
    if (i < 0 || i >= MAX_TURNOUT_LIST) return 0;
    return turnoutListState_[i];
}

// ── Routes ──────────────────────────────────────────────────────────────────

void ServerDataStore::setRouteListSize(int size) {
    routeListSize_ = size;
}

void ServerDataStore::addRouteEntry(int index, const String &sysName, const String &userName, int state) {
    if (index < 0 || index >= MAX_ROUTE_LIST || index >= routeListSize_) return;
    routeListIndex_[index]    = index;
    routeListSysName_[index]  = sysName;
    routeListUserName_[index] = userName;
    routeListState_[index]    = state;
}

const String &ServerDataStore::routeSysName(int i) const {
    if (i < 0 || i >= MAX_ROUTE_LIST) return empty_;
    return routeListSysName_[i];
}

const String &ServerDataStore::routeUserName(int i) const {
    if (i < 0 || i >= MAX_ROUTE_LIST) return empty_;
    return routeListUserName_[i];
}

int ServerDataStore::routeState(int i) const {
    if (i < 0 || i >= MAX_ROUTE_LIST) return 0;
    return routeListState_[i];
}

// ── Comparator ──────────────────────────────────────────────────────────────

int ServerDataStore::compareStrings(const void *a, const void *b) {
    const char *s1 = *(const char **)a;
    const char *s2 = *(const char **)b;
    return strcmp(s1, s2);
}
