// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteBlock.hpp"
#include "ByteBlock_fwd.hpp"
#include "ByteBlockEditor_fwd.hpp"

#include "impl/ByteBlockData_fwd.hpp"
#include "impl/UnsafeByteBlockBuffer_fwd.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/ByteRange.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <cstdint>
#include <span>
#include <vector>

namespace erbsland::mem {

/// An owning mutable byte block editor with copy-on-write storage.
/// Use this type to create and modify byte sequences.
/// Constructing an editor from a read-only `ByteBlock` always copies its visible bytes.
/// @tested{ByteBlockTest}
class ByteBlockEditor final {
    friend class ByteBlock;
    friend class impl::UnsafeByteBlockBuffer;

public:
    /// Create a byte block filled with the given byte value.
    /// @param length The number of bytes.
    /// @param value The byte value used to fill the block.
    explicit ByteBlockEditor(unit::ByteLength length, Byte value = Byte{});
    /// Create a byte block from byte values.
    explicit ByteBlockEditor(std::span<const Byte> bytes);
    /// Create a byte block from unsigned byte values.
    explicit ByteBlockEditor(std::span<const uint8_t> bytes);
    /// Create a byte block from char values.
    explicit ByteBlockEditor(std::span<const char> bytes);
    /// Create a byte block from byte values.
    explicit ByteBlockEditor(const std::vector<Byte> &bytes);
    /// Create a byte block from unsigned byte values.
    explicit ByteBlockEditor(const std::vector<uint8_t> &bytes);
    /// Create a byte block from char values.
    explicit ByteBlockEditor(const std::vector<char> &bytes);
    /// Create an editable copy of a read-only byte block.
    /// @param block The visible byte sequence to copy.
    explicit ByteBlockEditor(const ByteBlock &block);

    ByteBlockEditor();
    ~ByteBlockEditor();
    ByteBlockEditor(const ByteBlockEditor &);
    ByteBlockEditor(ByteBlockEditor &&) noexcept;
    auto operator=(const ByteBlockEditor &) -> ByteBlockEditor &;
    auto operator=(ByteBlockEditor &&) noexcept -> ByteBlockEditor &;

public: // comparison
    [[nodiscard]] auto operator<=>(const ByteBlockEditor &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const ByteBlockEditor &other, other);
    [[nodiscard]] auto operator<=>(const ByteBlock &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const ByteBlock &other, other);

public: // tests
    /// Test if this block contains no bytes.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return length().isZero(); }
    /// Test if this block starts with another byte sequence.
    [[nodiscard]] auto startsWith(const ByteBlock &other) const noexcept -> bool;
    /// Test if this block ends with another byte sequence.
    [[nodiscard]] auto endsWith(const ByteBlock &other) const noexcept -> bool;
    /// Test if this block contains another byte sequence.
    [[nodiscard]] auto contains(const ByteBlock &other) const noexcept -> bool;

public: // read
    /// Get the length of this byte block.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength;
    /// Get the index after the last byte.
    [[nodiscard]] auto endIndex() const noexcept -> unit::ByteIndex { return unit::ByteIndex::end(length()); }
    /// Get a byte or return a default value when the index is out of range.
    [[nodiscard]] auto get(unit::ByteIndex index, Byte defaultValue = Byte{}) const noexcept -> Byte;
    /// Get a byte or throw when the index is out of range.
    /// @throws err::OutOfRangeError If the index is out of range.
    [[nodiscard]] auto getOrThrow(unit::ByteIndex index) const -> Byte;

public: // write
    /// Set a byte value, ignoring out-of-range indexes.
    void set(unit::ByteIndex index, Byte value);
    /// Set a byte value or throw when the index is out of range.
    /// @throws err::OutOfRangeError If the index is out of range.
    void setOrThrow(unit::ByteIndex index, Byte value);

public: // slice
    /// Return a read-only slice of this editor.
    [[nodiscard]] auto slice(unit::ByteRange range) const noexcept -> ByteBlock;
    /// Return a slice from the given start to the given end.
    [[nodiscard]] auto slice(unit::ByteIndex begin, unit::ByteIndex end) const noexcept -> ByteBlock;
    /// Return a slice from the given start with the given length.
    [[nodiscard]] auto slice(unit::ByteIndex begin, unit::ByteLength length) const noexcept -> ByteBlock;

public: // find
    /// Find the first occurrence of a byte sequence.
    [[nodiscard]] auto find(const ByteBlock &bytes) const noexcept -> unit::ByteIndex;
    /// Find the first occurrence of a byte sequence at or after `start`.
    [[nodiscard]] auto find(const ByteBlock &bytes, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// Find the last occurrence of a byte sequence.
    [[nodiscard]] auto findLast(const ByteBlock &bytes) const noexcept -> unit::ByteIndex;

public: // modifiers
    /// Remove all bytes while preserving capacity.
    auto clear() noexcept -> ByteBlockEditor &;
    /// Reset this byte block and release all storage.
    void reset() noexcept;
    /// Remove a range of bytes.
    auto remove(unit::ByteRange range) -> ByteBlockEditor &;
    /// Keep only a range of bytes.
    auto keep(unit::ByteRange range) -> ByteBlockEditor &;
    /// Replace a range of bytes with another byte sequence.
    auto replace(unit::ByteRange range, const ByteBlock &replacement) -> ByteBlockEditor &;
    /// Insert a byte sequence at an index. Out-of-range indexes append at the end.
    auto insert(unit::ByteIndex index, const ByteBlock &bytes) -> ByteBlockEditor &;
    /// Append a single byte.
    auto append(Byte value) -> ByteBlockEditor &;
    /// Append a byte sequence.
    auto append(const ByteBlock &bytes) -> ByteBlockEditor &;
    /// Return a copy with a range removed.
    [[nodiscard]] auto removed(unit::ByteRange range) const -> ByteBlockEditor;
    /// Return a copy with a range replaced.
    [[nodiscard]] auto replaced(unit::ByteRange range, const ByteBlock &replacement) const -> ByteBlockEditor;
    /// Join byte sequences with this block as separator.
    [[nodiscard]] auto join(const std::vector<ByteBlock> &parts) const -> ByteBlockEditor;

public: // conversion
    /// Create a vector with the byte values.
    [[nodiscard]] auto toByteVector() const -> std::vector<Byte>;
    /// Create a vector with raw unsigned byte values.
    [[nodiscard]] auto toUInt8Vector() const -> std::vector<uint8_t>;
    /// Create a vector with raw char values.
    [[nodiscard]] auto toCharVector() const -> std::vector<char>;

public: // memory
    /// Detach shared storage.
    void detach();
    /// Get the current capacity.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength;
    /// Reserve capacity.
    void reserve(unit::ByteLength capacity);
    /// Shrink storage to the current length.
    void shrinkToFit();

public: // factory methods
    /// Join byte sequences without a separator.
    [[nodiscard]] static auto fromJoined(const std::vector<ByteBlock> &parts) -> ByteBlockEditor;

private:
    /// Create a block from already prepared shared storage.
    explicit ByteBlockEditor(impl::ByteBlockDataPtr data) noexcept;
    /// Ensure storage can hold at least `requiredCapacity` bytes and is not shared.
    void ensureCapacity(std::size_t requiredCapacity);
    /// Ensure non-null unique storage for the current size.
    void ensureUnique();
    /// Create data with the given size and capacity.
    [[nodiscard]] static auto createData(std::size_t size, std::size_t capacity) -> impl::ByteBlockDataPtr;
    /// Create data from a byte span.
    [[nodiscard]] static auto createData(std::span<const Byte> bytes) -> impl::ByteBlockDataPtr;
    /// Get a mutable data pointer.
    [[nodiscard]] auto dataForWrite() -> impl::ByteBlockData *;

private:
    impl::ByteBlockDataPtr _data; ///< Shared byte storage.
};

}
