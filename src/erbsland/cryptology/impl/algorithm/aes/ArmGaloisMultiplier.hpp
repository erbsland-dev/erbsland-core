// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GaloisMultiplier.hpp"

namespace erbsland::cryptology::impl {

/// ARM64 PMULL implementation of GHASH field multiplication.
/// The carry-less product is reduced with the SP 800-38D field polynomial.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
class ArmGaloisMultiplier final : public GaloisMultiplier {
public:
    // defaults/deletions
    ArmGaloisMultiplier() = default;
    ~ArmGaloisMultiplier() override = default;
    ArmGaloisMultiplier(const ArmGaloisMultiplier &) = delete;
    ArmGaloisMultiplier(ArmGaloisMultiplier &&) = delete;
    auto operator=(const ArmGaloisMultiplier &) -> ArmGaloisMultiplier & = delete;
    auto operator=(ArmGaloisMultiplier &&) -> ArmGaloisMultiplier & = delete;

public: // implement GaloisMultiplier
    /// @copydoc GaloisMultiplier::multiply()
    [[nodiscard]] auto multiply(const Block &left, const Block &right) const noexcept -> Block override;
    /// @copydoc GaloisMultiplier::isHardwareAccelerated()
    [[nodiscard]] auto isHardwareAccelerated() const noexcept -> bool override { return true; }
};

}
