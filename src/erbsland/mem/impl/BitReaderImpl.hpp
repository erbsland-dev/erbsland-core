// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BitOrder.hpp"
#include "../ByteBlock.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace erbsland::mem::impl {

/// Internal interface for a bit reader with a fixed bit order.
/// @tested{BitReaderTest}
class BitReaderImpl {
public:
    /// Create the implementation over an owned byte block.
    explicit BitReaderImpl(ByteBlock data, std::size_t bitPosition) noexcept;

    /// Erase retained input bits.
    virtual ~BitReaderImpl();

    // defaults/deletions
    BitReaderImpl(const BitReaderImpl &) = delete;
    BitReaderImpl(BitReaderImpl &&) = delete;
    auto operator=(const BitReaderImpl &) -> BitReaderImpl & = delete;
    auto operator=(BitReaderImpl &&) -> BitReaderImpl & = delete;

public:
    /// Replace consumed input and preserve the small unread tail.
    void refill(ByteBlock data);
    /// Get the fixed bit order.
    [[nodiscard]] virtual auto bitOrder() const noexcept -> BitOrder = 0;
    /// Get the total readable bit count.
    [[nodiscard]] auto bitCount() const noexcept -> std::size_t { return byteCount() * 8U; }
    /// Get the current bit position.
    [[nodiscard]] auto bitPosition() const noexcept -> std::size_t { return _bitPosition; }
    /// Set the clamped bit position.
    void setBitPosition(std::size_t bitPosition) noexcept;
    /// Get the total readable byte count.
    [[nodiscard]] auto byteCount() const noexcept -> std::size_t { return _prefixSize + _data.size(); }
    /// Get the current byte position.
    [[nodiscard]] auto bytePosition() const noexcept -> std::size_t { return _bitPosition / 8U; }
    /// Set the clamped byte position.
    void setBytePosition(std::size_t bytePosition) noexcept;
    /// Get the number of bytes touched by the cursor.
    [[nodiscard]] auto consumedByteCount() const noexcept -> std::size_t { return (_bitPosition + 7U) / 8U; }
    /// Get the remaining readable bit count.
    [[nodiscard]] auto remainingBitCount() const noexcept -> std::size_t { return bitCount() - _bitPosition; }
    /// Test if the cursor reached the end.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool { return _bitPosition >= bitCount(); }
    /// Test if the cursor is byte-aligned.
    [[nodiscard]] auto isByteAligned() const noexcept -> bool { return (_bitPosition % 8U) == 0U; }
    /// Test if a complete bit field is available.
    [[nodiscard]] auto canRead(std::size_t bitCount) const noexcept -> bool;
    /// Test if complete bytes are available.
    [[nodiscard]] auto canReadBytes(std::size_t byteCount) const noexcept -> bool;
    /// Advance by a clamped bit count.
    void advance(std::size_t bitCount) noexcept;
    /// Advance by a clamped byte count.
    void advanceBytes(std::size_t byteCount) noexcept;
    /// Advance to the next byte boundary.
    void alignToByte() noexcept;
    /// Read a field or return a fallback without advancing.
    [[nodiscard]] auto readBits(std::size_t bitCount, uint64_t defaultOnError) noexcept -> uint64_t;
    /// Read a field or throw if unavailable.
    [[nodiscard]] auto readBitsOrThrow(std::size_t bitCount) -> uint64_t;

    /// Read a byte-aligned field, sharing the input block or copying a retained prefix.
    [[nodiscard]] auto readBytesOrThrow(unit::ByteLength length) -> ByteBlock;

protected:
    /// Read a byte from the retained tail or the shared input block.
    [[nodiscard]] auto byteAt(std::size_t index) const noexcept -> Byte {
        return index < _prefixSize ? _prefix[index] : _data[index - _prefixSize];
    }
    /// Read a field known to be available.
    [[nodiscard]] virtual auto readBitsUnchecked(std::size_t bitCount) noexcept -> uint64_t = 0;

protected:
    std::array<Byte, 9U> _prefix{}; ///< At most 64 unread bits, including their initial bit offset.
    std::size_t _prefixSize{};      ///< Retained physical bytes.
    ByteBlock _block;               ///< Shared ownership of the input bytes.
    ConstByteSpan _data;            ///< Borrowed view backed by `_block`.
    std::size_t _bitPosition{};     ///< Position of the next bit.
};

}
