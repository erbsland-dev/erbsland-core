#pragma once

#include "../core/Definitions.hpp"

namespace erbsland::cryptology {

/// Persistent storage capabilities, independent of the current authorization state.
/// @tested{SecureStorageTest}
struct SecureStorageCapabilities final {
    bool supported{false};       ///< Persistent storage is implemented on this platform.
    bool authorizationUi{false}; ///< Explicit interaction opt-in can display authorization UI.
};

}
