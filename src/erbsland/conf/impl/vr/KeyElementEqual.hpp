// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfKey.hpp"

#include "../../../text/CaseSensitivity.hpp"

namespace erbsland::conf::impl {

/// Compares key elements with a compile-time case-sensitivity policy.
template <text::CaseSensitivity::Value tCaseSensitivity>
struct KeyElementEqual final {
    /// Test key elements for equality using the configured case sensitivity.
    [[nodiscard]] auto operator()(const text::String &lhs, const text::String &rhs) const noexcept -> bool {
        return lhs.compare(rhs, text::CaseSensitivity{tCaseSensitivity}.asciiComparisonFn()) ==
            std::strong_ordering::equal;
    }
};

}
