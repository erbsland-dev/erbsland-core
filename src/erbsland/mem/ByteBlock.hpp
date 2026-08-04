// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteArray.hpp"
#include "ByteBlock_fwd.hpp"
#include "ByteBlockEditor_fwd.hpp"
#include "ByteBuffer.hpp"
#include "ByteSpan.hpp"

#include "impl/ByteBlockData_fwd.hpp"
#include "impl/ByteDataView.hpp"
#include "impl/ByteReadTools.hpp"
#include "impl/UnsafeByteBlockAccess_fwd.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/ByteRange.hpp"
#include "../util/impl/ComparisonHelper.hpp"
#include "../util/LoopResult.hpp"

#include <compare>
#include <concepts>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>

namespace erbsland::mem {

/// An owning read-only byte block with shared copy-on-write storage.
/// Use this type to store, read and pass byte sequences.
/// A `ByteBlockEditor` is implicitly convertible to a `ByteBlock` without copying data.
/// Copying, moving and slicing are fast and copy-free operations.
/// @tested{ByteBlockTest}
class ByteBlock final {
    friend class ByteBlockEditor;
    friend class impl::UnsafeByteBlockAccess;

public:
    /// Create a byte block filled with the given byte value.
    /// @param length The number of bytes.
    /// @param value The byte value used to fill the block.
    explicit ByteBlock(unit::ByteLength length, Byte value = Byte{});
    /// Create a byte block by copying explicit byte values.
    explicit ByteBlock(std::initializer_list<Byte> bytes);
    /// Create a byte block by copying a fixed byte array.
    /// @tparam N The number of bytes.
    /// @param bytes The fixed byte array.
    template <std::size_t N>
    explicit ByteBlock(const ByteArray<N> &bytes) : ByteBlock{fromSpan(bytes.span())} {}
    /// Create a read-only byte block sharing the editor's data.
    /// @param editor The byte block editor whose complete data is shared.
    ByteBlock(const ByteBlockEditor &editor) noexcept; // NOLINT(*-explicit-constructor)

    // defaults
    ByteBlock();
    ~ByteBlock();
    ByteBlock(const ByteBlock &);
    ByteBlock(ByteBlock &&) noexcept;
    auto operator=(const ByteBlock &) -> ByteBlock &;
    auto operator=(ByteBlock &&) noexcept -> ByteBlock &;

public: // main operations
    /// Test if this block is marked as sensitive.
    [[nodiscard]] auto isSensitive() const noexcept -> bool;
    /// Permanently mark this block as sensitive.
    /// Marking empty blocks as sensitive does nothing.
    void markAsSensitive() noexcept;
    /// Return a shared read-only slice of this block.
    [[nodiscard]] auto slice(unit::ByteRange range) const noexcept -> ByteBlock;
    /// Return a slice from the given start to the given end.
    [[nodiscard]] auto slice(unit::ByteIndex begin, unit::ByteIndex end) const noexcept -> ByteBlock;
    /// Return a slice from the given start with the given length.
    [[nodiscard]] auto slice(unit::ByteIndex begin, unit::ByteLength length) const noexcept -> ByteBlock;
    /// Securely erase this block while preserving its length.
    void secureErase();

public: // comparison
    /// Compare this block with an editable byte block.
    [[nodiscard]] auto operator<=>(const ByteBlockEditor &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const ByteBlockEditor &other, other);
    /// Compare this block with another read-only byte block.
    [[nodiscard]] auto operator<=>(const ByteBlock &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const ByteBlock &other, other);
    /// Test equality without content-dependent short-circuiting.
    /// Equal-length inputs always inspect every byte; a length mismatch returns immediately.
    /// @param other The byte block to compare.
    /// @return `true` if both blocks have the same length and contents.
    [[nodiscard]] auto isEqualConstTime(const ByteBlock &other) const noexcept -> bool;
    /// @overload
    /// @param other The borrowed byte sequence to compare.
    /// @return `true` if both sequences have the same length and contents.
    [[nodiscard]] auto isEqualConstTime(ConstByteSpan other) const noexcept -> bool;

public: // tests
    /// Test if this block contains no bytes.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Test if this block starts with another byte sequence.
    [[nodiscard]] auto startsWith(const ByteBlock &other) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto startsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto startsWith(ConstByteSpan byteSequence) const noexcept -> bool;
    /// @overload
    template <std::size_t N>
    [[nodiscard]] auto startsWith(FixedConstByteSpan<N> byteSequence) const noexcept -> bool {
        return startsWith(ConstByteSpan{byteSequence});
    }
    /// Test if this block ends with another byte sequence.
    [[nodiscard]] auto endsWith(const ByteBlock &other) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto endsWith(std::initializer_list<Byte> byteSequence) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto endsWith(ConstByteSpan byteSequence) const noexcept -> bool;
    /// @overload
    template <std::size_t N>
    [[nodiscard]] auto endsWith(FixedConstByteSpan<N> byteSequence) const noexcept -> bool {
        return endsWith(ConstByteSpan{byteSequence});
    }
    /// Test if this block contains another byte sequence.
    [[nodiscard]] auto contains(const ByteBlock &other) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto contains(std::initializer_list<Byte> byteSequence) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto contains(ConstByteSpan byteSequence) const noexcept -> bool;
    /// @overload
    template <std::size_t N>
    [[nodiscard]] auto contains(FixedConstByteSpan<N> byteSequence) const noexcept -> bool {
        return contains(ConstByteSpan{byteSequence});
    }

public: // read
    /// Get the length of this block.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength;
    /// Get the index after the last byte.
    [[nodiscard]] auto endIndex() const noexcept -> unit::ByteIndex { return unit::ByteIndex::end(length()); }
    /// Get a byte or return a default value when the index is out of range.
    [[nodiscard]] auto get(unit::ByteIndex index, Byte defaultValue = Byte{}) const noexcept -> Byte;
    /// Get a byte or throw when the index is out of range.
    /// @throws err::OutOfRangeError If the index is out of range.
    [[nodiscard]] auto getOrThrow(unit::ByteIndex index) const -> Byte;
    /// Access all visible bytes through a read-only borrowed span.
    [[nodiscard]] auto span() const noexcept -> ConstByteSpan { return dataView().dataSpan(); }
    /// Access a clamped visible range through a read-only borrowed span.
    [[nodiscard]] auto span(unit::ByteRange range) const noexcept -> ConstByteSpan {
        return impl::ByteReadTools{dataView()}.span(range);
    }
    /// Access a clamped visible range through a read-only borrowed span.
    [[nodiscard]] auto span(unit::ByteIndex index, unit::ByteLength lengthValue) const noexcept -> ConstByteSpan {
        return span(unit::ByteRange{index, lengthValue});
    }
    /// Invoke a callback for every visible byte and its optional index.
    template <typename Function>
    auto forEach(Function function) const -> util::LoopResult {
        return impl::ByteReadTools{dataView()}.forEach(std::move(function));
    }

public: // integers
    /// Get an integer or return a default value if its byte range is invalid.
    /// @tparam T A non-boolean native integer type.
    /// @param offset The first byte index.
    /// @param endianness The byte order.
    /// @param defaultOnError The value returned for an invalid range.
    /// @return The decoded value, or `defaultOnError`.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] auto getInteger(
        const unit::ByteIndex offset,
        const Endianness endianness = Endianness::Little,
        const T defaultOnError = T{}) const noexcept -> T {
        return impl::ByteReadTools{dataView()}.getInteger<T>(offset, endianness, defaultOnError);
    }
    /// Get an integer or throw if its byte range is invalid.
    /// @tparam T A non-boolean native integer type.
    /// @param offset The first byte index.
    /// @param endianness The byte order.
    /// @return The decoded value.
    /// @throws err::OutOfRangeError If the integer does not fit at `offset`.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] auto getIntegerOrThrow(
        const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) const -> T {
        return impl::ByteReadTools{dataView()}.getIntegerOrThrow<T>(offset, endianness);
    }
    /// Decode an integer into an existing value, leaving it unchanged for an invalid range.
    /// @tparam T A non-boolean native integer type.
    /// @param value The destination value.
    /// @param offset The first byte index.
    /// @param endianness The byte order.
    /// @return `true` on success.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] auto getIntegerInto(
        T &value, const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) const noexcept
        -> bool {
        return impl::ByteReadTools{dataView()}.getIntegerInto(value, offset, endianness);
    }

public: // find
    /// Find the first occurrence of a byte sequence.
    [[nodiscard]] auto find(const ByteBlock &bytes) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(std::initializer_list<Byte> byteSequence) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(ConstByteSpan byteSequence) const noexcept -> unit::ByteIndex;
    /// @overload
    template <std::size_t N>
    [[nodiscard]] auto find(FixedConstByteSpan<N> byteSequence) const noexcept -> unit::ByteIndex {
        return find(ConstByteSpan{byteSequence});
    }
    /// Find the first occurrence of a byte sequence at or after `start`.
    [[nodiscard]] auto find(const ByteBlock &bytes, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(std::initializer_list<Byte> byteSequence, unit::ByteIndex start) const noexcept
        -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto find(ConstByteSpan byteSequence, unit::ByteIndex start) const noexcept -> unit::ByteIndex;
    /// @overload
    template <std::size_t N>
    [[nodiscard]] auto find(FixedConstByteSpan<N> byteSequence, unit::ByteIndex start) const noexcept
        -> unit::ByteIndex {
        return find(ConstByteSpan{byteSequence}, start);
    }
    /// Find the last occurrence of a byte sequence.
    [[nodiscard]] auto findLast(const ByteBlock &bytes) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto findLast(std::initializer_list<Byte> byteSequence) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto findLast(ConstByteSpan byteSequence) const noexcept -> unit::ByteIndex;
    /// @overload
    template <std::size_t N>
    [[nodiscard]] auto findLast(FixedConstByteSpan<N> byteSequence) const noexcept -> unit::ByteIndex {
        return findLast(ConstByteSpan{byteSequence});
    }

public: // conversion
    /// Create a deep-copying byte buffer with the visible values.
    [[nodiscard]] auto toByteBuffer() const -> ByteBuffer;
    /// Create a vector with raw unsigned byte values.
    [[nodiscard]] auto toUInt8Vector() const -> std::vector<uint8_t>;
    /// Create a vector with raw char values.
    [[nodiscard]] auto toCharVector() const -> std::vector<char>;

public: // factory methods
    /// Create a block by copying an Erbsland byte span.
    [[nodiscard]] static auto fromSpan(ConstByteSpan bytes) -> ByteBlock;
    /// Create a block by copying standard byte values.
    [[nodiscard]] static auto fromSpan(std::span<const std::byte> bytes) -> ByteBlock;
    /// Create a block by copying unsigned byte values.
    [[nodiscard]] static auto fromSpan(std::span<const uint8_t> bytes) -> ByteBlock;
    /// Create a block by copying character byte values.
    [[nodiscard]] static auto fromSpan(std::span<const char> bytes) -> ByteBlock;
    /// Create a block by copying unsigned byte values.
    [[nodiscard]] static auto fromVector(const std::vector<uint8_t> &bytes) -> ByteBlock;
    /// Create a block by copying character byte values.
    [[nodiscard]] static auto fromVector(const std::vector<char> &bytes) -> ByteBlock;

private:
    /// Create a block from shared data and a storage range.
    ByteBlock(impl::ByteBlockDataPtr data, unit::ByteRange range) noexcept;
    /// Get a borrowed internal view of the visible bytes.
    [[nodiscard]] auto dataView() const noexcept -> impl::ByteDataView;
    /// Get a unique ID for tests and diagnostics.
    [[nodiscard]] auto storageId() const noexcept -> std::size_t;

private:
    impl::ByteBlockDataPtr _data;                     ///< Shared byte storage.
    unit::ByteRange _range{unit::ByteRange::empty()}; ///< The visible range in the byte storage.
};

}
