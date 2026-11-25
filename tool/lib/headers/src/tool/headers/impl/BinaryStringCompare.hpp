// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

namespace tool::headers::impl {

/// Order internal path spellings by their unsigned UTF-8 bytes.
class BinaryStringCompare final {
public:
    /// Compare already normalized path keys without decoding their common prefix.
    /// @param left The left key.
    /// @param right The right key.
    /// @return Whether the left key precedes the right key.
    [[nodiscard]] auto operator()(const el::String &left, const el::String &right) const noexcept -> bool {
        return left.compareBinary(right) < 0;
    }
};

}
