// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PortablePoly1305.hpp"

#include <array>
#include <cstdint>

namespace erbsland::cryptology::impl {

/// x86-64 SSE2 four-block batching backend for RFC 8439 Poly1305.
/// @tested{ChaCha20BackendFullTest}
class X86Poly1305 final : public PortablePoly1305 {
public:
    /// Create an authenticator from the RFC 8439 256-bit one-time key.
    explicit X86Poly1305(mem::ConstByteSpan key) noexcept : PortablePoly1305{key} {}

    // defaults/deletions
    ~X86Poly1305() noexcept override = default;
    X86Poly1305(const X86Poly1305 &) = delete;
    X86Poly1305(X86Poly1305 &&) = delete;
    auto operator=(const X86Poly1305 &) -> X86Poly1305 & = delete;
    auto operator=(X86Poly1305 &&) -> X86Poly1305 & = delete;

protected:
    /// Process four complete message blocks per batching step while preserving RFC 8439 message order.
    void processFullBlocks(mem::ConstByteSpan blocks, std::size_t blockCount) noexcept override;
    /// Process one block with SSE2-paired limb products and the shared portable reduction.
    void processBlock(mem::ConstByteSpan block, bool complete) noexcept override;

private:
    /// Multiply five pairs of 26-bit limbs with SSE2 and return their 64-bit sum.
    [[nodiscard]] static auto multiplyFive(
        const std::array<uint64_t, 5> &left, const std::array<uint64_t, 5> &right) noexcept -> uint64_t;
};

}
