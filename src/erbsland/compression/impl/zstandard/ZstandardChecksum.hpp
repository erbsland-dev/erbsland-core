// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteArray.hpp"

#include <bit>

namespace erbsland::compression::impl {

/// Incremental xxHash64 checksum for Zstandard frames.
/// @tested{CompressionStreamingTest}
class ZstandardChecksum final {
private:
    static constexpr auto cXxHashPrime1 = uint64_t{11'400'714'785'074'694'791ULL}; ///< xxHash64 prime 1.
    static constexpr auto cXxHashPrime2 = uint64_t{14'029'467'366'897'019'727ULL}; ///< xxHash64 prime 2.
    static constexpr auto cXxHashPrime3 = uint64_t{1'609'587'929'392'839'161ULL};  ///< xxHash64 prime 3.
    static constexpr auto cXxHashPrime4 = uint64_t{9'650'029'242'287'828'579ULL};  ///< xxHash64 prime 4.
    static constexpr auto cXxHashPrime5 = uint64_t{2'870'177'450'012'600'261ULL};  ///< xxHash64 prime 5.

public:
    /// Add a bounded byte span.
    void update(mem::ConstByteSpan input) noexcept;
    /// Get the checksum without changing its state.
    auto value() const noexcept -> uint64_t;

private:
    /// Read a little-endian 64-bit lane.
    static auto read64(mem::ConstByteSpan input, std::size_t position) noexcept -> uint64_t;
    /// Read a little-endian 32-bit tail.
    static auto read32(mem::ConstByteSpan input, std::size_t position) noexcept -> uint32_t;
    /// Mix one lane into an accumulator.
    static auto round(uint64_t accumulator, uint64_t input) noexcept -> uint64_t;

private:
    std::array<uint64_t, 4U> _state{
        cXxHashPrime1 + cXxHashPrime2, cXxHashPrime2, 0U, uint64_t{} - cXxHashPrime1}; ///< Stripes.
    mem::ByteArray<32U> _tail;                                                         ///< Incomplete stripe.
    std::size_t _used{};                                                               ///< Tail length.
    uint64_t _length{};                                                                ///< Total input length.
};
}
