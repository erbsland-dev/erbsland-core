// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MoveAwareTestValue.hpp"

#include <cstddef>
#include <functional>

namespace erbsland::test {

/// Hashes a move-aware test value by its stored integer.
/// @notest{Used by tests in hash-based containers.}
struct MoveAwareTestValueHash final {
    /// Calculate the hash of a test value.
    [[nodiscard]] auto operator()(const MoveAwareTestValue &value) const noexcept -> std::size_t {
        return std::hash<int>{}(value.value());
    }
};

}
