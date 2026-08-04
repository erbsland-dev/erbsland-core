// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::json {

/// The semantic type of a JSON value.
enum class JsonType : uint8_t {
    Null,   ///< The null value.
    Bool,   ///< A boolean value.
    Number, ///< An integer or floating-point number.
    Text,   ///< A Unicode string.
    Array,  ///< An ordered array.
    Object, ///< A string-keyed object.
};

}
