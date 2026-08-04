// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PortablePoly1305.hpp"

#include <array>
#include <cstdint>

namespace erbsland::cryptology::impl {

/// ARM64 NEON four-block batching backend for RFC 8439 Poly1305.
/// @tested{ChaCha20BackendFullTest}
class ArmPoly1305 final : public PortablePoly1305 {
public:
    /// Create an authenticator from the RFC 8439 256-bit one-time key.
    explicit ArmPoly1305(mem::ConstByteSpan key) noexcept : PortablePoly1305{key} {}

    // defaults/deletions
    ~ArmPoly1305() noexcept override = default;
    ArmPoly1305(const ArmPoly1305 &) = delete;
    ArmPoly1305(ArmPoly1305 &&) = delete;
    auto operator=(const ArmPoly1305 &) -> ArmPoly1305 & = delete;
    auto operator=(ArmPoly1305 &&) -> ArmPoly1305 & = delete;

protected:
    /// Process four complete message blocks per batching step while preserving RFC 8439 message order.
    void processFullBlocks(mem::ConstByteSpan blocks, std::size_t blockCount) noexcept override;
    /// Process one block with NEON-paired limb products and the shared portable reduction.
    void processBlock(mem::ConstByteSpan block, bool complete) noexcept override;

private:
    /// Multiply five pairs of 26-bit limbs with NEON and return their 64-bit sum.
    [[nodiscard]] static auto multiplyFive(
        const std::array<uint64_t, 5> &left, const std::array<uint64_t, 5> &right) noexcept -> uint64_t;
};

}
