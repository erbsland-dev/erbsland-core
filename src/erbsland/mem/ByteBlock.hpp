// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteBlock_fwd.hpp"
#include "ByteBlockEditor_fwd.hpp"

#include "impl/ByteBlockData_fwd.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/ByteRange.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <cstdint>
#include <span>
#include <vector>

namespace erbsland::mem {

/// An owning read-only byte block with shared copy-on-write storage.
/// Use this type to store, read and pass byte sequences.
/// A `ByteBlockEditor` is implicitly convertible to a `ByteBlock` without copying data.
/// Copying, moving and slicing are fast and copy-free operations.
/// @tested{ByteBlockTest}
class ByteBlock final {
    friend class ByteBlockEditor;

public:
    /// Create a byte block filled with the given byte value.
    /// @param length The number of bytes.
    /// @param value The byte value used to fill the block.
    explicit ByteBlock(unit::ByteLength length, Byte value = Byte{});
    /// Create a byte block by copying byte values.
    explicit ByteBlock(std::span<const Byte> bytes);
    /// Create a byte block by copying unsigned byte values.
    explicit ByteBlock(std::span<const uint8_t> bytes);
    /// Create a byte block by copying char values.
    explicit ByteBlock(std::span<const char> bytes);
    /// Create a byte block by copying byte values.
    explicit ByteBlock(const std::vector<Byte> &bytes);
    /// Create a byte block by copying unsigned byte values.
    explicit ByteBlock(const std::vector<uint8_t> &bytes);
    /// Create a byte block by copying char values.
    explicit ByteBlock(const std::vector<char> &bytes);
    /// Create a read-only byte block sharing the editor's data.
    /// @param editor The byte block editor whose complete data is shared.
    ByteBlock(const ByteBlockEditor &editor) noexcept; // NOLINT(*-explicit-constructor)

    ByteBlock();
    ~ByteBlock();
    ByteBlock(const ByteBlock &);
    ByteBlock(ByteBlock &&) noexcept;
    auto operator=(const ByteBlock &) -> ByteBlock &;
    auto operator=(ByteBlock &&) noexcept -> ByteBlock &;

public: // comparison
    [[nodiscard]] auto operator<=>(const ByteBlockEditor &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const ByteBlockEditor &other, other);
    [[nodiscard]] auto operator<=>(const ByteBlock &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const ByteBlock &other, other);

public: // tests
    /// Test if this block contains no bytes.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Test if this block starts with another byte sequence.
    [[nodiscard]] auto startsWith(const ByteBlock &other) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto startsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto startsWith(std::span<const Byte> byteSequence) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto startsWith(const std::vector<Byte> &byteSequence) const noexcept -> bool;
    /// Test if this block ends with another byte sequence.
    [[nodiscard]] auto endsWith(const ByteBlock &other) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto endsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto endsWith(const std::vector<Byte> &byteSequence) const noexcept -> bool;
    /// Test if this block contains another byte sequence.
    [[nodiscard]] auto contains(const ByteBlock &other) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto contains(std::initializer_list<Byte> byteSequence) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto contains(const std::vector<Byte> &byteSequence) const noexcept -> bool;

public: // read
    /// Access the visible bytes as a contiguous read-only span.
    [[nodiscard]] auto bytes() const noexcept -> std::span<const Byte> { return dataSpan(); }
    /// Get the length of this block.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength;
    /// Get the index after the last byte.
    [[nodiscard]] auto endIndex() const noexcept -> unit::ByteIndex { return unit::ByteIndex::end(length()); }
    /// Get a byte or return a default value when the index is out of range.
    [[nodiscard]] auto get(unit::ByteIndex index, Byte defaultValue = Byte{}) const noexcept -> Byte;
    /// Get a byte or throw when the index is out of range.
    /// @throws err::OutOfRangeError If the index is out of range.
    [[nodiscard]] auto getOrThrow(unit::ByteIndex index) const -> Byte;

public: // slice
    /// Return a shared read-only slice of this block.
    [[nodiscard]] auto slice(unit::ByteRange range) const noexcept -> ByteBlock;
    /// Return a slice from the given start to the given end.
    [[nodiscard]] auto slice(unit::ByteIndex begin, unit::ByteIndex end) const noexcept -> ByteBlock;
    /// Return a slice from the given start with the given length.
    [[nodiscard]] auto slice(unit::ByteIndex begin, unit::ByteLength length) const noexcept -> ByteBlock;

public: // find
    /// Find the first occurrence of a byte sequence.
    [[nodiscard]] auto find(const ByteBlock &bytes) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(std::initializer_list<Byte> byteSequence) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(const std::vector<Byte> &byteSequence) const noexcept -> unit::ByteIndex;
    /// Find the first occurrence of a byte sequence at or after `start`.
    [[nodiscard]] auto find(const ByteBlock &bytes, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(std::initializer_list<Byte> byteSequence, unit::ByteIndex start) const noexcept
        -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(const std::vector<Byte> &byteSequence, unit::ByteIndex start) const noexcept
        -> unit::ByteIndex;
    /// Find the last occurrence of a byte sequence.
    [[nodiscard]] auto findLast(const ByteBlock &bytes) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto findLast(std::initializer_list<Byte> byteSequence) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto findLast(const std::vector<Byte> &byteSequence) const noexcept -> unit::ByteIndex;

public: // conversion
    /// Create a vector with the byte values.
    [[nodiscard]] auto toByteVector() const -> std::vector<Byte>;
    /// Create a vector with raw unsigned byte values.
    [[nodiscard]] auto toUInt8Vector() const -> std::vector<uint8_t>;
    /// Create a vector with raw char values.
    [[nodiscard]] auto toCharVector() const -> std::vector<char>;

private:
    /// Create a block from shared data and a storage range.
    ByteBlock(impl::ByteBlockDataPtr data, unit::ByteRange range) noexcept;
    /// Get the visible byte span.
    [[nodiscard]] auto dataSpan() const noexcept -> std::span<const Byte>;
    /// Get a unique ID for tests and diagnostics.
    [[nodiscard]] auto storageId() const noexcept -> std::size_t;
    /// Find a byte sequence within the block starting from a given position.
    [[nodiscard]] auto findImpl(std::span<const Byte> needle, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// Find a byte sequence within the block starting from the end.
    [[nodiscard]] auto findLastImpl(std::span<const Byte> needle) const noexcept -> unit::ByteIndex;

private:
    impl::ByteBlockDataPtr _data;                     ///< Shared byte storage.
    unit::ByteRange _range{unit::ByteRange::empty()}; ///< The visible range in the byte storage.
};

}
