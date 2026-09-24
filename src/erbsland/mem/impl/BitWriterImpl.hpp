// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BitOrder.hpp"
#include "../ByteBlockEditor.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::mem::impl {

/// Internal interface for a bit writer with a fixed bit order.
/// @tested{BitWriterTest}
class BitWriterImpl {
public:
    // defaults/deletions
    BitWriterImpl() = default;
    virtual ~BitWriterImpl() = default;
    BitWriterImpl(const BitWriterImpl &) = delete;
    BitWriterImpl(BitWriterImpl &&) = delete;
    auto operator=(const BitWriterImpl &) -> BitWriterImpl & = delete;
    auto operator=(BitWriterImpl &&) -> BitWriterImpl & = delete;

public:
    /// Get the fixed bit order.
    [[nodiscard]] virtual auto bitOrder() const noexcept -> BitOrder = 0;
    /// Get the logical written bit count.
    [[nodiscard]] auto bitCount() const noexcept -> std::size_t { return _bitCount; }
    /// Get the current bit position.
    [[nodiscard]] auto bitPosition() const noexcept -> std::size_t { return _bitPosition; }
    /// Set the clamped bit position.
    void setBitPosition(std::size_t bitPosition) noexcept;
    /// Get the physical output byte count.
    [[nodiscard]] auto byteCount() const noexcept -> std::size_t { return byteCountForBits(_bitCount); }
    /// Get the current byte position.
    [[nodiscard]] auto bytePosition() const noexcept -> std::size_t { return _bitPosition / 8U; }
    /// Set the clamped byte position.
    void setBytePosition(std::size_t bytePosition) noexcept;
    /// Get the number of bytes touched by the cursor.
    [[nodiscard]] auto consumedByteCount() const noexcept -> std::size_t { return byteCountForBits(_bitPosition); }
    /// Get the written bit count after the cursor.
    [[nodiscard]] auto remainingBitCount() const noexcept -> std::size_t { return _bitCount - _bitPosition; }
    /// Test if the cursor is at the logical end.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool { return _bitPosition >= _bitCount; }
    /// Test if the cursor is byte-aligned.
    [[nodiscard]] auto isByteAligned() const noexcept -> bool { return (_bitPosition % 8U) == 0U; }
    /// Advance by a clamped bit count.
    void advance(std::size_t bitCount) noexcept;
    /// Advance by a clamped byte count.
    void advanceBytes(std::size_t byteCount) noexcept;
    /// Zero-pad to the next byte boundary.
    void alignToByte();
    /// Protect all output allocations for the remaining writer lifetime.
    void markAsSensitive() noexcept {
        _sensitive = true;
        _block.markAsSensitive();
    }
    /// Discard the output and release storage.
    void reset() noexcept;
    /// Reserve physical storage for a bit capacity.
    void reserveBits(std::size_t bitCapacity);
    /// Reserve physical storage for a byte capacity.
    void reserveBytes(std::size_t byteCapacity);
    /// Write a field and advance.
    void writeBits(uint64_t value, std::size_t bitCount);
    /// Write an aligned byte field transactionally, including aliased input.
    void writeBytes(ConstByteSpan bytes);
    /// Share the physical output bytes.
    [[nodiscard]] auto toByteBlock() const noexcept -> ByteBlock;
    /// Transfer the physical output bytes and reset.
    [[nodiscard]] auto takeByteBlockEditor() noexcept -> ByteBlockEditor;

protected:
    /// Write a validated field using the fixed bit order.
    [[nodiscard]] virtual auto writeBitsUnchecked(uint64_t value, std::size_t bitCount) -> std::size_t = 0;
    /// Acquire a writable span after bounded storage preparation.
    [[nodiscard]] auto writableSpan() -> ByteSpan { return _block.writableSpan(); }
    /// Ensure a write fits addressable storage.
    void prepareWrite(std::size_t bitCount);
    /// Convert a logical bit count to a physical byte count.
    [[nodiscard]] static auto byteCountForBits(std::size_t bitCount) noexcept -> std::size_t {
        return bitCount / 8U + (bitCount % 8U != 0U ? 1U : 0U);
    }

protected:
    bool _sensitive{};          ///< Secure-erasure policy for current and future storage.
    ByteBlockEditor _block;     ///< Bytes containing the written bits.
    std::size_t _bitCount{};    ///< Logical number of written bits.
    std::size_t _bitPosition{}; ///< Position of the next write.
};

}
