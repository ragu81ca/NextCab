// ServerDataStore.h — Owns roster, turnout, and route data received from server
// Replaces 18+ global arrays that were scattered across WiTcontroller.ino
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h>

// Size limits (previously #defines in WiTcontroller.h)
static constexpr int MAX_ROSTER      = 70;
static constexpr int MAX_TURNOUT_LIST = 60;
static constexpr int MAX_ROUTE_LIST   = 60;

// Compile-time config guards (may be defined in config_buttons.h / static.h)
#ifndef ROSTER_SORT_SEQUENCE
#define ROSTER_SORT_SEQUENCE 0
#endif

class ServerDataStore {
public:
    ServerDataStore() = default;

    // ── Roster ──────────────────────────────────────────────────────────
    int  rosterSize() const { return rosterSize_; }
    void setRosterSize(int size);
    void addRosterEntry(int index, const String &name, int address, char length);
    void sortRoster();                       // applies ROSTER_SORT_SEQUENCE

    int          rosterSortedIndex(int i) const;
    const String &rosterName(int i) const;
    int          rosterAddress(int i) const;
    char         rosterLength(int i) const;

    // Lookup: find roster name for a DCC address (for DISPLAY_LOCO_NAME)
    String findRosterNameByAddress(int address) const;

    // ── Turnouts ────────────────────────────────────────────────────────
    int  turnoutListSize() const { return turnoutListSize_; }
    void setTurnoutListSize(int size);
    void addTurnoutEntry(int index, const String &sysName, const String &userName, int state);

    const String &turnoutSysName(int i) const;
    const String &turnoutUserName(int i) const;
    int           turnoutState(int i) const;

    // ── Routes ──────────────────────────────────────────────────────────
    int  routeListSize() const { return routeListSize_; }
    void setRouteListSize(int size);
    void addRouteEntry(int index, const String &sysName, const String &userName, int state);

    const String &routeSysName(int i) const;
    const String &routeUserName(int i) const;
    int           routeState(int i) const;

    // ── Turnout/Route prefixes (server-type dependent) ──────────────────
    const String &turnoutPrefix() const { return turnoutPrefix_; }
    void setTurnoutPrefix(const String &p) { turnoutPrefix_ = p; }
    const String &routePrefix() const { return routePrefix_; }
    void setRoutePrefix(const String &p) { routePrefix_ = p; }

private:
    static const String empty_;

    // Roster
    int    rosterSize_ = 0;
    int    rosterIndex_[MAX_ROSTER];
    int    rosterSortedIndex_[MAX_ROSTER];
    String rosterName_[MAX_ROSTER];
    int    rosterAddress_[MAX_ROSTER];
    char   rosterLength_[MAX_ROSTER];
    char   rosterSortStrings_[MAX_ROSTER][14];
    char*  rosterSortPointers_[MAX_ROSTER];

    // Turnouts
    int    turnoutListSize_ = 0;
    int    turnoutListIndex_[MAX_TURNOUT_LIST];
    String turnoutListSysName_[MAX_TURNOUT_LIST];
    String turnoutListUserName_[MAX_TURNOUT_LIST];
    int    turnoutListState_[MAX_TURNOUT_LIST];

    // Routes
    int    routeListSize_ = 0;
    int    routeListIndex_[MAX_ROUTE_LIST];
    String routeListSysName_[MAX_ROUTE_LIST];
    String routeListUserName_[MAX_ROUTE_LIST];
    int    routeListState_[MAX_ROUTE_LIST];

    // Prefixes
    String turnoutPrefix_;
    String routePrefix_;

    // qsort comparator (static for C linkage)
    static int compareStrings(const void *a, const void *b);
};
