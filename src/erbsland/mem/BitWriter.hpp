// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BitOrder.hpp"
#include "BitWriter_fwd.hpp"
#include "Byte.hpp"
#include "ByteBlock.hpp"
#include "ByteBlockEditor.hpp"

#include "impl/BitWriter_fwd.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace erbsland::mem {

/// A move-only sequential writer for bit fields.
/// The selected bit order is fixed for the lifetime of the writer. A moved-from writer may only be destroyed or
/// assigned another writer.
/// @seedoc{/reference/mem/memory_and_byte_data}
/// @tested{BitWriterTest}
class BitWriter final {
public:
    /// Create an empty writer with the selected bit order.
    explicit BitWriter(BitOrder bitOrder = BitOrder::MostSignificantFirst);

    /// Destroy the writer.
    ~BitWriter();
    /// Move a writer.
    BitWriter(BitWriter &&) noexcept;
    /// Move-assign a writer.
    auto operator=(BitWriter &&) noexcept -> BitWriter &;

    // defaults/deletions
    BitWriter(const BitWriter &) = delete;
    auto operator=(const BitWriter &) -> BitWriter & = delete;

public: // accessors
    /// Get the immutable bit order.
    [[nodiscard]] auto bitOrder() const noexcept -> BitOrder;
    /// Get the logical number of written bits.
    [[nodiscard]] auto bitCount() const noexcept -> std::size_t;
    /// Get the current bit position.
    [[nodiscard]] auto bitPosition() const noexcept -> std::size_t;
    /// Set the bit position, clamped to `bitCount()`.
    void setBitPosition(std::size_t bitPosition) noexcept;
    /// Get the physical byte count containing the written bits.
    [[nodiscard]] auto byteCount() const noexcept -> std::size_t;
    /// Get the byte containing the current bit position.
    [[nodiscard]] auto bytePosition() const noexcept -> std::size_t;
    /// Set the current position to the start of a byte, clamped to the logical end.
    void setBytePosition(std::size_t bytePosition) noexcept;
    /// Get the number of bytes touched by the current position.
    [[nodiscard]] auto consumedByteCount() const noexcept -> std::size_t;
    /// Get the number of written bits after the current position.
    [[nodiscard]] auto remainingBitCount() const noexcept -> std::size_t;
    /// Test whether the writer is positioned at its logical end.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool;
    /// Test whether the current position is byte-aligned.
    [[nodiscard]] auto isByteAligned() const noexcept -> bool;
    /// Advance within the written bits, clamped to the logical end.
    void advance(std::size_t bitCount) noexcept;
    /// Advance by complete bytes, clamped to the logical end.
    void advanceBytes(std::size_t byteCount) noexcept;
    /// Write zero bits up to the next byte boundary.
    auto alignToByte() -> BitWriter &;
    /// Permanently protect current and future output allocations, including after reset or transfer.
    void markAsSensitive() noexcept;
    /// Discard all output and release its storage.
    void reset() noexcept;

public: // write
    /// Reserve storage for at least the requested number of bits.
    auto reserveBits(std::size_t bitCapacity) -> BitWriter &;
    /// Reserve storage for at least the requested number of bytes.
    auto reserveBytes(std::size_t byteCapacity) -> BitWriter &;
    /// Write the requested low-order bits and advance.
    /// @throws err::OutOfRangeError If `bitCount` exceeds 64 or the position overflows.
    auto writeBits(uint64_t value, std::size_t bitCount) -> BitWriter &;
    /// Write one bit and advance.
    auto writeBool(bool value) -> BitWriter &;
    /// Write eight consecutive bits and advance.
    auto writeByte(Byte value) -> BitWriter &;
    /// Write a byte-aligned field without changing the selected bit order.
    /// @param bytes Bytes to overwrite or append at the current position; aliased input is supported.
    /// @return This writer.
    /// @throws err::OutOfRangeError If unaligned or the bit position would overflow.
    auto writeBytes(ConstByteSpan bytes) -> BitWriter &;
    /// Return the physical bytes containing all written bits.
    [[nodiscard]] auto toByteBlock() const noexcept -> ByteBlock;
    /// Take the physical output bytes and reset the writer.
    [[nodiscard]] auto takeByteBlockEditor() noexcept -> ByteBlockEditor;

private:
    std::unique_ptr<impl::BitWriterImpl> _impl; ///< Order-specialized implementation.
};

}
