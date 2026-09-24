// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock.hpp"

#include <cstdint>

namespace erbsland::compression::impl {

/// Result of the Burrows-Wheeler transform for one Bzip2 block.
/// @tested{ByteCompressionTest}
struct Bzip2BwtResult final {
    mem::ByteBlock lastColumn;
    uint32_t originalPointer{};
};

}
