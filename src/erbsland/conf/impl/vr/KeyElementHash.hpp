// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfKey.hpp"

#include "../../../text/CaseSensitivity.hpp"

namespace erbsland::conf::impl {

/// Hashes a key element with a compile-time case-sensitivity policy.
template <text::CaseSensitivity::Value tCaseSensitivity>
struct KeyElementHash final {
    /// Hash one key element using the configured case sensitivity.
    [[nodiscard]] auto operator()(const text::String &element) const noexcept -> std::size_t {
        return ConfKey::elementHash(element, tCaseSensitivity);
    }
};

}
