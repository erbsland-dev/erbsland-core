// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::impl {

/// Kind of sequence value represented by an FSE table.
enum class ZstandardSequenceCode : uint8_t {
    LiteralLength, ///< Literal count preceding a match.
    Offset,        ///< Backward match offset.
    MatchLength,   ///< Number of bytes copied from a match.
};

}
