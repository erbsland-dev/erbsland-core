// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfKey.hpp"
#include "KeyIndexData_fwd.hpp"

namespace erbsland::conf::impl {

/// The storage interface for a key index.
class KeyIndexData {
public:
    // defaults
    virtual ~KeyIndexData() = default;

    /// Test whether an entire key is present.
    [[nodiscard]] virtual auto hasKey(const ConfKey &key) const noexcept -> bool = 0;

    /// Test whether a key element is present at an index.
    [[nodiscard]] virtual auto hasKeyElement(const text::String &element, std::size_t index) const noexcept -> bool = 0;

    /// Insert a key if it is not present.
    [[nodiscard]] virtual auto tryAddKey(const ConfKey &key) -> bool = 0;
};

}
