// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>

namespace erbsland::compression::impl {

/// One candidate match in the Zstandard encoder history.
/// @tested{ZstandardInternalTest}
struct ZstandardMatch final {
    std::size_t position{}; ///< Referenced absolute input position.
    std::size_t length{};   ///< Match length.
};

}
