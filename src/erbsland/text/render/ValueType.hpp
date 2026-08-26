// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::render {

/// The semantic type of a render value.
enum class ValueType : uint8_t {
    Null,     ///< A null value.
    Boolean,  ///< A boolean value.
    Text,     ///< A text value.
    Integer,  ///< A signed integer value.
    Float,    ///< A floating point value.
    List,     ///< A list of values.
    Map,      ///< A map of values.
    Callback, ///< A lazily evaluated callback value.
};

}
