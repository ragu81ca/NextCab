// WiThrottleProtocol.h stub for native unit tests.
// Provides the Direction enum and constants that the production code references.
#pragma once

#include <cstdint>

// Direction enum — matches the real WiThrottleProtocol library definition.
enum Direction {
    Reverse = 0,
    Forward = 1
};

// Maximum number of functions per decoder.
#ifndef MAX_FUNCTIONS
#define MAX_FUNCTIONS 32
#endif

// Maximum number of multi-throttles.
#ifndef MAX_WIT_THROTTLES
#define MAX_WIT_THROTTLES 6
#endif

// Forward declaration — full class not needed for MomentumController tests.
class WiThrottleProtocol {};
