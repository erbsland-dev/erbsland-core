// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfKey.hpp"

#include "../../../text/CaseSensitivity.hpp"

namespace erbsland::conf::impl {

/// Compares complete keys with a compile-time case-sensitivity policy.
template <text::CaseSensitivity::Value tCaseSensitivity>
struct KeyEqual final {
    /// Test complete keys for equality using the configured case sensitivity.
    [[nodiscard]] auto operator()(const ConfKey &lhs, const ConfKey &rhs) const noexcept -> bool {
        return lhs.isEqual(rhs, tCaseSensitivity);
    }
};

}
