// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteBlock_fwd.hpp"

#include "impl/ByteBlockData.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/ByteRange.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <cstdint>
#include <span>
#include <vector>

namespace erbsland::mem {

/// A read-only shared view into byte block data.
/// @tested{ByteBlockTest}
class ByteBlockView final {
    friend class ByteBlock;

public:
    /// Create a view to the full byte block.
    /// @param block The byte block to view.
    ByteBlockView(const ByteBlock &block) noexcept; // NOLINT(*-explicit-constructor)

    ByteBlockView() = default;
    ~ByteBlockView() = default;
    ByteBlockView(const ByteBlockView &) = default;
    ByteBlockView(ByteBlockView &&) = default;
    auto operator=(const ByteBlockView &) -> ByteBlockView & = default;
    auto operator=(ByteBlockView &&) -> ByteBlockView & = default;

public: // comparison
    [[nodiscard]] auto operator<=>(const ByteBlock &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const ByteBlock &other, other);
    [[nodiscard]] auto operator<=>(const ByteBlockView &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const ByteBlockView &other, other);

public: // tests
    /// Test if the view contains no bytes.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Test if this view starts with another byte sequence.
    [[nodiscard]] auto startsWith(const ByteBlockView &other) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto startsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto startsWith(const std::vector<Byte> &byteSequence) const noexcept -> bool;
    /// Test if this view ends with another byte sequence.
    [[nodiscard]] auto endsWith(const ByteBlockView &other) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto endsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto endsWith(const std::vector<Byte> &byteSequence) const noexcept -> bool;
    /// Test if this view contains another byte sequence.
    [[nodiscard]] auto contains(const ByteBlockView &other) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto contains(std::initializer_list<Byte> byteSequence) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto contains(const std::vector<Byte> &byteSequence) const noexcept -> bool;

public: // read
    /// Access the visible bytes as a contiguous read-only span.
    [[nodiscard]] auto bytes() const noexcept -> std::span<const Byte> { return dataSpan(); }
    /// Get the length of this view.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength;
    /// Get the index after the last byte.
    [[nodiscard]] auto endIndex() const noexcept -> unit::ByteIndex { return unit::ByteIndex::end(length()); }
    /// Get a byte or return a default value when the index is out of range.
    [[nodiscard]] auto get(unit::ByteIndex index, Byte defaultValue = Byte{}) const noexcept -> Byte;
    /// Get a byte or throw when the index is out of range.
    /// @throws err::OutOfRangeError If the index is out of range.
    [[nodiscard]] auto getOrThrow(unit::ByteIndex index) const -> Byte;

public: // slice
    /// Return a slice of this view.
    [[nodiscard]] auto slice(unit::ByteRange range) const noexcept -> ByteBlockView;
    /// Return a slice from the given start to the given end.
    [[nodiscard]] auto slice(unit::ByteIndex begin, unit::ByteIndex end) const noexcept -> ByteBlockView;
    /// Return a slice from the given start with the given length.
    [[nodiscard]] auto slice(unit::ByteIndex begin, unit::ByteLength length) const noexcept -> ByteBlockView;

public: // find
    /// Find the first occurrence of a byte sequence.
    [[nodiscard]] auto find(const ByteBlockView &bytes) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(std::initializer_list<Byte> byteSequence) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(const std::vector<Byte> &byteSequence) const noexcept -> unit::ByteIndex;
    /// Find the first occurrence of a byte sequence at or after `start`.
    [[nodiscard]] auto find(const ByteBlockView &bytes, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(std::initializer_list<Byte> byteSequence, unit::ByteIndex start) const noexcept
        -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(const std::vector<Byte> &byteSequence, unit::ByteIndex start) const noexcept
        -> unit::ByteIndex;
    /// Find the last occurrence of a byte sequence.
    [[nodiscard]] auto findLast(const ByteBlockView &bytes) const noexcept -> unit::ByteIndex;
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
    /// Create a view from shared data and a storage range.
    ByteBlockView(impl::ByteBlockDataPtr data, unit::ByteRange range) noexcept;
    /// Get the visible byte span.
    [[nodiscard]] auto dataSpan() const noexcept -> std::span<const Byte>;
    /// Get a unique ID for tests and diagnostics.
    [[nodiscard]] auto storageId() const noexcept -> std::size_t;
    /// Find a byte sequence within the view starting from a given position.
    [[nodiscard]] auto findImpl(std::span<const Byte> needle, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// Find a byte sequence within the view starting from the end.
    [[nodiscard]] auto findLastImpl(std::span<const Byte> needle) const noexcept -> unit::ByteIndex;

private:
    impl::ByteBlockDataPtr _data;                     ///< Shared byte storage.
    unit::ByteRange _range{unit::ByteRange::empty()}; ///< The visible range in the byte storage.
};

}
