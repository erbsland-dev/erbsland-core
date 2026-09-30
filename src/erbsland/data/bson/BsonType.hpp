// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::data::bson {
/// Semantic types supported by a BSON value.
enum class BsonType : uint8_t {
    Null,     ///< Null.
    Bool,     ///< Boolean.
    Int32,    ///< Signed 32-bit integer.
    Int64,    ///< Signed 64-bit integer.
    Double,   ///< 64-bit floating point.
    Text,     ///< UTF-8 string.
    Binary,   ///< Binary payload and subtype.
    DateTime, ///< UTC millisecond date/time.
    Array,    ///< Ordered array.
    Document, ///< Named document fields.
    Opaque,   ///< Known BSON type retained without interpretation.
};
}
