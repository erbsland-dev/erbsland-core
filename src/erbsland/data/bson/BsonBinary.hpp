// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteBlock.hpp"

#include <cstdint>

namespace erbsland::data::bson {
/// BSON binary payload and its subtype.
/// @tested{BsonValueTest}
struct BsonBinary final {
    mem::ByteBlock bytes; ///< Payload bytes.
    uint8_t subtype{};    ///< BSON binary subtype.
};
}
