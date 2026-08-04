// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GaloisMultiplier.hpp"

namespace erbsland::cryptology::impl {

/// Portable fixed-iteration GHASH field multiplication.
/// This directly implements NIST SP 800-38D, Algorithm 1, without secret-dependent branches.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest}
class PortableGaloisMultiplier final : public GaloisMultiplier {
public:
    // defaults/deletions
    PortableGaloisMultiplier() = default;
    ~PortableGaloisMultiplier() override = default;
    PortableGaloisMultiplier(const PortableGaloisMultiplier &) = delete;
    PortableGaloisMultiplier(PortableGaloisMultiplier &&) = delete;
    auto operator=(const PortableGaloisMultiplier &) -> PortableGaloisMultiplier & = delete;
    auto operator=(PortableGaloisMultiplier &&) -> PortableGaloisMultiplier & = delete;

public: // implement GaloisMultiplier
    /// @copydoc GaloisMultiplier::multiply()
    [[nodiscard]] auto multiply(const Block &left, const Block &right) const noexcept -> Block override;
    /// @copydoc GaloisMultiplier::isHardwareAccelerated()
    [[nodiscard]] auto isHardwareAccelerated() const noexcept -> bool override { return false; }
};

}
