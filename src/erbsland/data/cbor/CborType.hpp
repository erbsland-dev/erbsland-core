// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::data::cbor {
/// The semantic type of a supported CBOR value.
enum class CborType : uint8_t {
    Null,     ///< Null.
    Bool,     ///< Boolean.
    Signed,   ///< Signed integer.
    Unsigned, ///< Unsigned integer.
    Float,    ///< Floating-point value.
    Text,     ///< UTF-8 text.
    Bytes,    ///< Binary bytes.
    Array,    ///< Ordered values.
    Map,      ///< String-keyed values.
    DateTime, ///< Tagged date/time.
    Link,     ///< DAG-CBOR CID link.
};
}
