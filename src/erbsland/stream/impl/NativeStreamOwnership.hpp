// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::stream::impl {

/// Defines if a native stream wrapper owns the native handle.
enum class NativeStreamOwnership : uint8_t {
    Borrowed, ///< Do not close the native handle.
    Owned,    ///< Close the native handle when the wrapper is destroyed.
};

}
