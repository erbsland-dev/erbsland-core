// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::mem {

/// The method used to clean up a shared array allocation before deallocation.
enum class SharedArrayDataCleanupMethod : uint8_t {
    None,        ///< Deallocate storage after normal element and header destruction.
    SecureErase, ///< Securely erase the complete allocation after destruction and before deallocation.
};

}
