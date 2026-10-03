#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::cryptology {

/// A persistent secret operation failure.
enum class SecureStorageErrorCategory : uint8_t {
    Unsupported,         ///< No supported persistent storage backend exists.
    AccessDenied,        ///< The operating system denied access.
    InteractionRequired, ///< Authorization requires forbidden interaction.
    Cancelled,           ///< The user cancelled authorization.
    SizeLimit,           ///< The portable 2,560-byte payload limit was exceeded.
    InvalidName,         ///< The namespace or item name is invalid.
    NativeFailure,       ///< The native provider failed for another reason.
};

}
