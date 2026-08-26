// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::render::impl {

/// One ordinary built-in layout filter encoded directly in private bytecode.
enum class BuiltInFilter : uint8_t {
    Capitalize,
    Lower,
    Upper,
    Trim,
    Replace,
    First,
    Last,
    Join,
    Length,
    Reverse,
    Sort,
    Keys,
    Values,
    Items,
    Absolute,
    Round,
    Sum,
    Minimum,
    Maximum,
    Default,
    ToJson,
    Count,
};

}
