// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PortableChaCha20Backend.hpp"

#include <arm_neon.h>

namespace erbsland::cryptology::impl {

/// ARM64 NEON four-block RFC 8439 ChaCha20 generator.
/// @tested{ChaCha20BackendFullTest}
class ArmChaCha20Backend final : public PortableChaCha20Backend {
public:
    /// Retain a validated key and nonce.
    ArmChaCha20Backend(mem::ConstByteSpan key, mem::ConstByteSpan nonce) noexcept :
        PortableChaCha20Backend{key, nonce} {}

    // defaults/deletions
    ~ArmChaCha20Backend() noexcept override = default;
    ArmChaCha20Backend(const ArmChaCha20Backend &) = delete;
    ArmChaCha20Backend(ArmChaCha20Backend &&) = delete;
    auto operator=(const ArmChaCha20Backend &) -> ArmChaCha20Backend & = delete;
    auto operator=(ArmChaCha20Backend &&) -> ArmChaCha20Backend & = delete;

public: // implement ChaCha20Backend
    /// Generate four blocks with lane-wise NEON operations, or use the scalar backend for shorter batches.
    [[nodiscard]] auto generateBlocks(uint32_t firstCounter, std::size_t blockCount) noexcept -> Batch override;

private:
    /// Rotate all four 32-bit lanes left.
    template <int tAmount>
    [[nodiscard]] static auto rotateLeft(uint32x4_t value) noexcept -> uint32x4_t;
    /// Apply one quarter round independently to four blocks.
    static void quarterRound(uint32x4_t &a, uint32x4_t &b, uint32x4_t &c, uint32x4_t &d) noexcept;
};

}
