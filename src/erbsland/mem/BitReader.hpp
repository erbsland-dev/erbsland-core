// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BitReader_fwd.hpp"
#include "ByteSpan.hpp"

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace erbsland::mem {

/// A sequential reader for individual bits in a read-only byte span.
/// Bits are read most-significant bit first within each byte. The reader borrows its input; the source bytes must
/// remain valid for the reader's lifetime.
/// @seedoc{/reference/mem/memory_and_byte_data}
/// @tested{BitReaderTest}
class BitReader final {
public:
    /// Create an empty reader.
    BitReader() = default;
    /// Create a reader over `data` at `bitPosition`.
    /// Positions beyond the input are clamped to `bitCount()`.
    explicit BitReader(ConstByteSpan data, std::size_t bitPosition = 0U) noexcept;

    // defaults
    ~BitReader() = default;
    BitReader(const BitReader &) = default;
    BitReader(BitReader &&) = default;
    auto operator=(const BitReader &) -> BitReader & = default;
    auto operator=(BitReader &&) -> BitReader & = default;

public: // accessors
    /// Get the total number of readable bits.
    [[nodiscard]] auto bitCount() const noexcept -> std::size_t { return _data.size() * 8U; }
    /// Get the current bit position.
    [[nodiscard]] auto bitPosition() const noexcept -> std::size_t { return _bitPosition; }
    /// Set the current bit position, clamped to `bitCount()`.
    void setBitPosition(std::size_t bitPosition) noexcept;
    /// Get the number of bits remaining at the current position.
    [[nodiscard]] auto remainingBitCount() const noexcept -> std::size_t { return bitCount() - _bitPosition; }
    /// Test if the reader is at the end of the input.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool { return _bitPosition >= bitCount(); }
    /// Test if `count` bits can be read at the current position.
    [[nodiscard]] auto canRead(std::size_t count) const noexcept -> bool { return count <= remainingBitCount(); }
    /// Advance by `count` bits, clamped to `bitCount()`.
    void advance(std::size_t count) noexcept;

public: // read
    /// Read the next bit as a boolean, or return `false` at end.
    [[nodiscard]] auto readBool() noexcept -> bool;
    /// Read the next bit as zero or one of the requested integer type, or return zero at end.
    /// @tparam T Any native integral type except `bool`.
    template <std::integral T>
        requires(!std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] auto readInteger() noexcept -> T {
        return static_cast<T>(readBool());
    }

private:
    ConstByteSpan _data;           ///< Borrowed read-only bytes.
    std::size_t _bitPosition = 0U; ///< Position of the next bit.
};

}
