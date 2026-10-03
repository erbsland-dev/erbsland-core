#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::cryptology {

/// Whether a persistent secret operation may display native authorization UI.
enum class SecureStorageInteraction : uint8_t {
    Forbid, ///< Fail rather than displaying authorization UI.
    Allow,  ///< Allow native authorization UI where the platform supports it.
};

}
