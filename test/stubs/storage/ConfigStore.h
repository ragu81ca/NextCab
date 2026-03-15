// ConfigStore.h stub for native unit tests.
// Only the LocoType enum is needed by MomentumController.
#pragma once

#include <cstdint>

enum class LocoType : uint8_t {
    Diesel   = 0,
    Steam    = 1,
    Electric = 2
};
