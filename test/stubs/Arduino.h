// Arduino.h stub for native (desktop) unit tests.
// Provides just enough of the Arduino API for core logic to compile
// without any ESP32 or hardware dependencies.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <string>

// ── Core Arduino types ─────────────────────────────────────────────────

using byte = uint8_t;
using boolean = bool;

// ── String ─────────────────────────────────────────────────────────────
// Minimal Arduino String shim backed by std::string.

class String : public std::string {
public:
    using std::string::string;   // inherit all std::string constructors
    String() = default;
    String(const std::string& s) : std::string(s) {}
    String(const char* s) : std::string(s ? s : "") {}
    String(int val)   : std::string(std::to_string(val)) {}
    String(float val) : std::string(std::to_string(val)) {}

    // Arduino-specific conversions
    const char* c_str() const { return std::string::c_str(); }
    int  toInt()   const { return std::stoi(*this); }
    int  length()  const { return (int)std::string::size(); }
    bool equals(const String& s) const { return *this == s; }
};

// ── millis() ───────────────────────────────────────────────────────────
// Default stub returns 0.  Tests inject their own clock via the
// MomentumController(ClockFn) constructor, so this is only needed for
// code that calls millis() at file scope or in constructors.

inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}

// ── min / max ──────────────────────────────────────────────────────────
// Arduino provides macro versions; we use template functions.

#ifndef min
template <typename T> T min(T a, T b) { return a < b ? a : b; }
#endif
#ifndef max
template <typename T> T max(T a, T b) { return a > b ? a : b; }
#endif

// ── Serial ─────────────────────────────────────────────────────────────
// Absorbs all Serial.print/println/printf calls silently.

struct SerialStub {
    void begin(unsigned long) {}
    void print(const char*) {}
    void print(int) {}
    void print(float, int = 2) {}
    void print(const String&) {}
    void println() {}
    void println(const char*) {}
    void println(int) {}
    void println(float, int = 2) {}
    void println(const String&) {}

    template<typename... Args>
    void printf(const char*, Args...) {}
};

inline SerialStub Serial;

// ── Misc Arduino macros ────────────────────────────────────────────────

#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW 0
#endif
#ifndef INPUT
#define INPUT 0
#endif
#ifndef OUTPUT
#define OUTPUT 1
#endif
#ifndef INPUT_PULLUP
#define INPUT_PULLUP 2
#endif

inline void pinMode(uint8_t, uint8_t) {}
inline int  digitalRead(uint8_t) { return LOW; }
inline void digitalWrite(uint8_t, uint8_t) {}
inline int  analogRead(uint8_t) { return 0; }
