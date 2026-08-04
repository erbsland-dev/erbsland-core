// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Poly1305.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace erbsland::cryptology::impl {

/// Portable RFC 8439 Poly1305 using five 26-bit limbs and 64-bit products.
/// @tested{ChaCha20PrimitiveTest ChaCha20BackendFullTest}
class PortablePoly1305 : public Poly1305 {
public:
    /// Create an authenticator from the RFC 8439 256-bit one-time key.
    /// @param key The first 16 bytes are clamped `r`; the final 16 bytes are `s`.
    explicit PortablePoly1305(mem::ConstByteSpan key) noexcept;

    ~PortablePoly1305() noexcept override;

    // defaults/deletions
    PortablePoly1305(const PortablePoly1305 &) = delete;
    PortablePoly1305(PortablePoly1305 &&) = delete;
    auto operator=(const PortablePoly1305 &) -> PortablePoly1305 & = delete;
    auto operator=(PortablePoly1305 &&) -> PortablePoly1305 & = delete;

public: // implement Poly1305
    void update(mem::ConstByteSpan data) noexcept override;
    [[nodiscard]] auto finalize() noexcept -> Tag override;
    void secureErase() noexcept override;

protected:
    /// Process consecutive complete 16-byte blocks.
    /// Architecture subclasses override this batching boundary while preserving the same recurrence.
    virtual void processFullBlocks(mem::ConstByteSpan blocks, std::size_t blockCount) noexcept;
    /// Process one padded or complete 16-byte block.
    virtual void processBlock(mem::ConstByteSpan block, bool complete) noexcept;
    /// Decode one RFC 8439 block and add it to the radix-2^26 accumulator.
    void addBlockToAccumulator(mem::ConstByteSpan block, bool complete) noexcept;
    /// Reduce five convolution products modulo `2^130 - 5` into the accumulator.
    void reduceProduct(std::array<uint64_t, 5> &product) noexcept;

protected:
    std::array<uint64_t, 5> _r{};  ///< Clamped multiplier limbs.
    std::array<uint64_t, 5> _r5{}; ///< Multiplier limbs one through four multiplied by five.
    std::array<uint64_t, 5> _h{};  ///< Accumulator limbs.
    mem::ByteArray<16> _pad;       ///< Additive `s` half of the one-time key.
    mem::ByteArray<16> _partial;   ///< Retained incomplete input block.
    std::size_t _partialLength{};  ///< Number of bytes retained in `_partial`.
};

}
