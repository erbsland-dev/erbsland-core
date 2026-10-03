// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace tool::idna {

/// Source-derived IDNA joining values.
/// @tested{UnicodeSourceTest PublicSuffixFullTest}
enum class IdnaJoining : uint8_t {
    Other, ///< No joining behavior relevant to IDNA.
    L,     ///< Left-joining.
    R,     ///< Right-joining.
    D,     ///< Dual-joining.
    T,     ///< Transparent.
};

}
