// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteArray.hpp"

namespace erbsland::cryptology::impl {

/// Internal GF(2^128) multiplication contract used by GHASH.
/// Multiplication follows NIST SP 800-38D, Section 6.3.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
class GaloisMultiplier {
public:
    /// The fixed 128-bit field element type.
    using Block = mem::ByteArray<16>;

public:
    // defaults/deletions
    GaloisMultiplier() = default;
    virtual ~GaloisMultiplier() = default;
    GaloisMultiplier(const GaloisMultiplier &) = delete;
    GaloisMultiplier(GaloisMultiplier &&) = delete;
    auto operator=(const GaloisMultiplier &) -> GaloisMultiplier & = delete;
    auto operator=(GaloisMultiplier &&) -> GaloisMultiplier & = delete;

public:
    /// Multiply two GHASH field elements.
    [[nodiscard]] virtual auto multiply(const Block &left, const Block &right) const noexcept -> Block = 0;
    /// Test whether this implementation uses polynomial-multiply CPU instructions.
    [[nodiscard]] virtual auto isHardwareAccelerated() const noexcept -> bool = 0;
};

}
