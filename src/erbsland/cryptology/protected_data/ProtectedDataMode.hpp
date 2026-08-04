// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// Select how application-scoped protected data is encrypted.
enum class ProtectedDataMode : uint8_t {
    Automatic,    ///< Prefer native protection and fall back to the internal provider during initialization.
    PlatformOnly, ///< Require the native platform provider.
    InternalOnly, ///< Use the bundled AES-256-GCM provider without probing native facilities.
};

}
