// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfKey.hpp"

#include "../../../text/CaseSensitivity.hpp"

namespace erbsland::conf::impl {

/// Hashes a complete key with a compile-time case-sensitivity policy.
template <text::CaseSensitivity::Value tCaseSensitivity>
struct KeyHash final {
    /// Hash a complete key using the configured case sensitivity.
    [[nodiscard]] auto operator()(const ConfKey &key) const noexcept -> std::size_t {
        return key.hash(tCaseSensitivity);
    }
};

}
