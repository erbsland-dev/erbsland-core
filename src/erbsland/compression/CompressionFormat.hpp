// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression {

/// The outer representation of compressed bytes.
enum class CompressionFormat : uint8_t {
    Raw,  ///< The algorithm's standalone representation.
    Zip,  ///< The payload representation used in a ZIP entry.
    Core, ///< The self-describing Erbsland Core compression envelope.
};

}
