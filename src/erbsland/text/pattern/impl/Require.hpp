// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../impl/ThrowHelper.hpp"

#include <string_view>
#include <type_traits>

namespace erbsland::text::pattern::impl {

/// Require a condition for static and parsed pattern construction.
/// @tested{StringPatternTest}
constexpr void requirePattern(const bool condition, const std::string_view reason) {
    if (condition) {
        return;
    }
    if (std::is_constant_evaluated()) {
        throw "Invalid string pattern";
    }
    text::impl::throwParseError(reason);
}

}
