// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::render::impl {

/// One argument-free built-in value test.
enum class ValueTest : uint8_t {
    Null,
    True,
    False,
    Boolean,
    Integer,
    Float,
    Number,
    Text,
    List,
    Map,
    Iterable,
    Scalar,
    Even,
    Odd,
    Count,
};

}
