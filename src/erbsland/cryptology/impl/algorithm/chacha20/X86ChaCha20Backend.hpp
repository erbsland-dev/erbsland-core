// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PortableChaCha20Backend.hpp"

#include <emmintrin.h>

namespace erbsland::cryptology::impl {

/// x86-64 SSE2 four-block RFC 8439 ChaCha20 generator.
/// @tested{ChaCha20BackendFullTest}
class X86ChaCha20Backend final : public PortableChaCha20Backend {
public:
    /// Retain a validated key and nonce.
    X86ChaCha20Backend(mem::ConstByteSpan key, mem::ConstByteSpan nonce) noexcept :
        PortableChaCha20Backend{key, nonce} {}

    // defaults/deletions
    ~X86ChaCha20Backend() noexcept override = default;
    X86ChaCha20Backend(const X86ChaCha20Backend &) = delete;
    X86ChaCha20Backend(X86ChaCha20Backend &&) = delete;
    auto operator=(const X86ChaCha20Backend &) -> X86ChaCha20Backend & = delete;
    auto operator=(X86ChaCha20Backend &&) -> X86ChaCha20Backend & = delete;

public: // implement ChaCha20Backend
    /// Generate four blocks with lane-wise SSE2 operations, or use the scalar backend for shorter batches.
    [[nodiscard]] auto generateBlocks(uint32_t firstCounter, std::size_t blockCount) noexcept -> Batch override;

private:
    /// Rotate all four 32-bit lanes left.
    [[nodiscard]] static auto rotateLeft(__m128i value, int amount) noexcept -> __m128i;
    /// Apply one quarter round independently to four blocks.
    static void quarterRound(__m128i &a, __m128i &b, __m128i &c, __m128i &d) noexcept;
};

}
