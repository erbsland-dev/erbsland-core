// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BitOrder.hpp"
#include "BitReader_fwd.hpp"
#include "Byte.hpp"
#include "ByteBlock.hpp"

#include "impl/BitReader_fwd.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace erbsland::mem {

/// A move-only sequential reader for fields in an owned byte block.
/// The selected bit order is fixed for the lifetime of the reader. A moved-from reader may only be destroyed or
/// assigned another reader.
/// @seedoc{/reference/mem/memory_and_byte_data}
/// @tested{BitReaderTest}
class BitReader final {
public:
    /// Create an empty most-significant-bit-first reader.
    BitReader();
    /// Create a reader that shares ownership of `data`.
    /// @param data The byte block containing the input bits.
    /// @param bitOrder The order used for bits within bytes and integer fields.
    /// @param bitPosition Initial bit position, clamped to `bitCount()`.
    explicit BitReader(
        ByteBlock data, BitOrder bitOrder = BitOrder::MostSignificantFirst, std::size_t bitPosition = 0U);

    /// Destroy the reader.
    ~BitReader();
    /// Move a reader.
    BitReader(BitReader &&) noexcept;
    /// Move-assign a reader.
    auto operator=(BitReader &&) noexcept -> BitReader &;

    // defaults/deletions
    BitReader(const BitReader &) = delete;
    auto operator=(const BitReader &) -> BitReader & = delete;

public: // input
    /// Replace consumed input while retaining up to 64 unread bits without allocating.
    /// The cursor becomes relative to the retained tail followed by `data`; its bit alignment is preserved.
    /// @param data The next byte block in the same continuous bit representation.
    /// @throws err::OutOfRangeError If more than 64 bits remain unread.
    void refill(ByteBlock data);

public: // accessors
    /// Get the immutable bit order.
    [[nodiscard]] auto bitOrder() const noexcept -> BitOrder;
    /// Get the total number of readable bits.
    [[nodiscard]] auto bitCount() const noexcept -> std::size_t;
    /// Get the current bit position.
    [[nodiscard]] auto bitPosition() const noexcept -> std::size_t;
    /// Set the current bit position, clamped to `bitCount()`.
    void setBitPosition(std::size_t bitPosition) noexcept;
    /// Get the total number of readable bytes.
    [[nodiscard]] auto byteCount() const noexcept -> std::size_t;
    /// Get the byte containing the current bit position.
    [[nodiscard]] auto bytePosition() const noexcept -> std::size_t;
    /// Set the current position to the start of a byte, clamped to the input.
    void setBytePosition(std::size_t bytePosition) noexcept;
    /// Get the number of bytes touched by the current bit position.
    [[nodiscard]] auto consumedByteCount() const noexcept -> std::size_t;
    /// Get the number of remaining readable bits.
    [[nodiscard]] auto remainingBitCount() const noexcept -> std::size_t;
    /// Test whether the reader reached the input end.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool;
    /// Test whether the current position is byte-aligned.
    [[nodiscard]] auto isByteAligned() const noexcept -> bool;
    /// Test whether a number of bits can be read.
    [[nodiscard]] auto canRead(std::size_t bitCount) const noexcept -> bool;
    /// Test whether a number of complete bytes can be read from the current bit position.
    [[nodiscard]] auto canReadBytes(std::size_t byteCount) const noexcept -> bool;
    /// Advance by bits, clamped to the input end.
    void advance(std::size_t bitCount) noexcept;
    /// Advance by complete bytes, clamped to the input end.
    void advanceBytes(std::size_t byteCount) noexcept;
    /// Advance to the next byte boundary.
    void alignToByte() noexcept;

public: // read
    /// Read up to 64 bits, or return `defaultOnError` without advancing if unavailable.
    [[nodiscard]] auto readBits(std::size_t bitCount, uint64_t defaultOnError = 0U) noexcept -> uint64_t;
    /// Read up to 64 bits.
    /// @throws err::OutOfRangeError If `bitCount` exceeds 64 or the field is unavailable.
    [[nodiscard]] auto readBitsOrThrow(std::size_t bitCount) -> uint64_t;
    /// Read one bit as a boolean, or return `false` at end.
    [[nodiscard]] auto readBool() noexcept -> bool;
    /// Read one bit as a boolean.
    /// @throws err::OutOfRangeError If the bit is unavailable.
    [[nodiscard]] auto readBoolOrThrow() -> bool;
    /// Read eight consecutive bits as a byte, or return `defaultOnError` without advancing if unavailable.
    [[nodiscard]] auto readByte(Byte defaultOnError = Byte{}) noexcept -> Byte;
    /// Read eight consecutive bits as a byte.
    /// @throws err::OutOfRangeError If the byte is unavailable.
    [[nodiscard]] auto readByteOrThrow() -> Byte;

    /// Read an exact byte-aligned field, sharing input storage when possible.
    /// @param length The finite number of bytes to read.
    /// @return The field; a field including a refill tail is copied into sensitive storage.
    /// @throws err::OutOfRangeError If unaligned, non-finite, or insufficient bytes are available.
    [[nodiscard]] auto readBytesOrThrow(unit::ByteLength length) -> ByteBlock;

private:
    std::unique_ptr<impl::BitReaderImpl> _impl; ///< Order-specialized implementation.
};

}
