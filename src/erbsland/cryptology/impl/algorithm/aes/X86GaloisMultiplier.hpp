// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GaloisMultiplier.hpp"

namespace erbsland::cryptology::impl {

/// x86-64 PCLMULQDQ implementation of GHASH field multiplication.
/// The carry-less product is reduced with the SP 800-38D field polynomial.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
class X86GaloisMultiplier final : public GaloisMultiplier {
public:
    // defaults/deletions
    X86GaloisMultiplier() = default;
    ~X86GaloisMultiplier() override = default;
    X86GaloisMultiplier(const X86GaloisMultiplier &) = delete;
    X86GaloisMultiplier(X86GaloisMultiplier &&) = delete;
    auto operator=(const X86GaloisMultiplier &) -> X86GaloisMultiplier & = delete;
    auto operator=(X86GaloisMultiplier &&) -> X86GaloisMultiplier & = delete;

public: // implement GaloisMultiplier
    /// @copydoc GaloisMultiplier::multiply()
    [[nodiscard]] auto multiply(const Block &left, const Block &right) const noexcept -> Block override;
    /// @copydoc GaloisMultiplier::isHardwareAccelerated()
    [[nodiscard]] auto isHardwareAccelerated() const noexcept -> bool override { return true; }
};

}
