// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteArray.hpp"
#include "ByteBlock.hpp"
#include "ByteBlock_fwd.hpp"
#include "ByteBlockEditor_fwd.hpp"
#include "ByteBuffer.hpp"
#include "ByteSpan.hpp"
#include "Endianness.hpp"

#include "impl/ByteBlockData_fwd.hpp"
#include "impl/ByteDataView.hpp"
#include "impl/ByteIntegerAccess.hpp"
#include "impl/ByteReadTools.hpp"
#include "impl/ByteWriteTools.hpp"
#include "impl/UnsafeByteBlockBuffer_fwd.hpp"

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
    /// Create a byte block by copying explicit byte values.
    explicit ByteBlockEditor(std::initializer_list<Byte> bytes);
    /// Create a byte block from a fixed byte array.
    /// @tparam N The number of bytes.
    /// @param bytes The fixed byte array.
    template <std::size_t N>
    explicit ByteBlockEditor(const ByteArray<N> &bytes) : ByteBlockEditor{fromSpan(bytes.span())} {}
    /// Create an editable copy of a read-only byte block.
    /// @param block The visible byte sequence to copy.
    explicit ByteBlockEditor(const ByteBlock &block);

    // defaults
    ByteBlockEditor();
    ~ByteBlockEditor();
    ByteBlockEditor(const ByteBlockEditor &);
    ByteBlockEditor(ByteBlockEditor &&) noexcept;
    auto operator=(const ByteBlockEditor &) -> ByteBlockEditor &;
    auto operator=(ByteBlockEditor &&) noexcept -> ByteBlockEditor &;

public: // main operations
    /// Create an independent copy containing the visible bytes.
    [[nodiscard]] auto copy() const -> ByteBlockEditor;
    /// Test if this allocation is marked as sensitive.
    [[nodiscard]] auto isSensitive() const noexcept -> bool;
    /// Permanently mark this allocation as sensitive.
    /// Marking a storage-less empty editor is a no-op.
    void markAsSensitive() noexcept;
    /// Return a read-only slice of this editor.
    [[nodiscard]] auto slice(unit::ByteRange range) const noexcept -> ByteBlock;
    /// Return a slice from the given start to the given end.
    [[nodiscard]] auto slice(unit::ByteIndex begin, unit::ByteIndex end) const noexcept -> ByteBlock;
    /// Return a slice from the given start with the given length.
    [[nodiscard]] auto slice(unit::ByteIndex begin, unit::ByteLength length) const noexcept -> ByteBlock;
    /// Remove all bytes while preserving capacity.
    auto clear() noexcept -> ByteBlockEditor &;
    /// Reset this byte block and release all storage.
    void reset() noexcept;
    /// Securely erase this editor while preserving length and capacity.
    void secureErase();
    /// Resize the block, zero-filling growth.
    auto resize(unit::ByteLength length) -> ByteBlockEditor &;
    /// Remove a range of bytes.
    auto remove(unit::ByteRange range) -> ByteBlockEditor &;
    /// Keep only a range of bytes.
    auto keep(unit::ByteRange range) -> ByteBlockEditor &;
    /// Replace a range of bytes with another byte sequence.
    auto replace(unit::ByteRange range, const ByteBlock &replacement) -> ByteBlockEditor &;
    /// Replace a range by copying a borrowed byte span.
    auto replace(unit::ByteRange range, ConstByteSpan replacement) -> ByteBlockEditor &;
    /// @overload
    template <std::size_t N>
    auto replace(unit::ByteRange range, FixedConstByteSpan<N> replacement) -> ByteBlockEditor & {
        return replace(range, ConstByteSpan{replacement});
    }
    /// Insert a byte sequence at an index. Out-of-range indexes append at the end.
    auto insert(unit::ByteIndex index, const ByteBlock &bytes) -> ByteBlockEditor &;
    /// Insert a borrowed byte span at an index.
    auto insert(unit::ByteIndex index, ConstByteSpan bytes) -> ByteBlockEditor &;
    /// @overload
    template <std::size_t N>
    auto insert(unit::ByteIndex index, FixedConstByteSpan<N> bytes) -> ByteBlockEditor & {
        return insert(index, ConstByteSpan{bytes});
    }
    /// Append one or more bytes.
    /// @param value The byte value to append.
    /// @param length The number of times to append the value.
    auto append(Byte value, unit::ByteLength length = unit::ByteLength::one()) -> ByteBlockEditor &;
    /// Append a byte sequence.
    auto append(const ByteBlock &bytes) -> ByteBlockEditor &;
    /// Append a borrowed byte span.
    auto append(ConstByteSpan bytes) -> ByteBlockEditor &;
    /// @overload
    template <std::size_t N>
    auto append(FixedConstByteSpan<N> bytes) -> ByteBlockEditor & {
        return append(ConstByteSpan{bytes});
    }
    /// Append an integer using the selected byte order.
    /// @tparam T A non-boolean native integer type.
    /// @param value The integer value.
    /// @param endianness The byte order.
    /// @return This editor.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    auto appendInteger(const T value, const Endianness endianness = Endianness::Little) -> ByteBlockEditor & {
        const auto offset = appendZeroed(unit::ByteLength{sizeof(T)});
        impl::ByteWriteTools{writableSpan()}.setIntegerOrThrow(offset, value, endianness);
        return *this;
    }
    /// Overwrite a clamped destination range with as many block bytes as fit.
    auto overwrite(unit::ByteRange range, const ByteBlock &bytes) -> ByteBlockEditor &;
    /// Overwrite from the beginning with as many source bytes as fit.
    auto overwrite(ConstByteSpan bytes) -> ByteBlockEditor &;
    /// Overwrite from an index with as many source bytes as fit.
    auto overwrite(unit::ByteIndex index, ConstByteSpan bytes) -> ByteBlockEditor &;
    /// Overwrite a clamped destination range with as many source bytes as fit.
    auto overwrite(unit::ByteRange range, ConstByteSpan bytes) -> ByteBlockEditor &;
    /// Fill all visible bytes.
    auto fill(Byte value) -> ByteBlockEditor &;
    /// Fill a clamped byte range.
    auto fill(unit::ByteRange range, Byte value) -> ByteBlockEditor &;
    /// XOR every byte with an equally sized block.
    /// @return `false` without changing this editor if the lengths differ.
    [[nodiscard]] auto xorWith(const ByteBlock &bytes) -> bool;
    /// XOR every byte with an equally sized block.
    /// @throws err::ParameterError If the lengths differ.
    auto xorWithOrThrow(const ByteBlock &bytes) -> ByteBlockEditor &;
    /// XOR every byte with an equally sized borrowed source.
    [[nodiscard]] auto xorWith(ConstByteSpan bytes) -> bool;
    /// XOR every byte with an equally sized borrowed source.
    /// @throws err::ParameterError If the lengths differ.
    auto xorWithOrThrow(ConstByteSpan bytes) -> ByteBlockEditor &;
    /// XOR a clamped destination range with as many source bytes as fit.
    auto xorWith(unit::ByteRange range, ConstByteSpan bytes) -> ByteBlockEditor &;
    /// Return a copy with a range removed.
    [[nodiscard]] auto removed(unit::ByteRange range) const -> ByteBlockEditor;
    /// Return an independent copy containing a clamped range of bytes.
    [[nodiscard]] auto kept(unit::ByteRange range) const -> ByteBlockEditor;
    /// Return a copy with a range replaced.
    [[nodiscard]] auto replaced(unit::ByteRange range, const ByteBlock &replacement) const -> ByteBlockEditor;
    /// Join byte sequences with this block as separator.
    [[nodiscard]] auto join(std::initializer_list<ByteBlock> parts) const -> ByteBlockEditor;

public: // comparison
    /// Compare this editor with another editor.
    [[nodiscard]] auto operator<=>(const ByteBlockEditor &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const ByteBlockEditor &other, other);
    /// Compare this editor with an immutable byte block.
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
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return length().isZero(); }
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
    /// Get the length of this byte block.
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

public: // write
    /// Set a byte value, ignoring out-of-range indexes.
    void set(unit::ByteIndex index, Byte value);
    /// Set a byte value or throw when the index is out of range.
    /// @throws err::OutOfRangeError If the index is out of range.
    void setOrThrow(unit::ByteIndex index, Byte value);
    /// XOR a byte value at an index, ignoring out-of-range indexes.
    void xorAt(unit::ByteIndex index, Byte value);
    /// XOR a byte value at an index or throw when the index is out of range.
    /// @throws err::OutOfRangeError If the index is out of range.
    void xorAtOrThrow(unit::ByteIndex index, Byte value);
    /// Store an integer, leaving the block unchanged if its byte range is invalid.
    /// @tparam T A non-boolean native integer type.
    /// @param offset The first byte index.
    /// @param value The integer value.
    /// @param endianness The byte order.
    /// @return `true` on success.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] auto setInteger(
        const unit::ByteIndex offset, const T value, const Endianness endianness = Endianness::Little) -> bool {
        if (!offset.isValid() || !impl::isIntegerByteRangeValid<T>(offset.toSizeT(), length().toSizeT())) {
            return false;
        }
        return impl::ByteWriteTools{writableSpan()}.setInteger(offset, value, endianness);
    }
    /// Store an integer or throw if its byte range is invalid.
    /// @tparam T A non-boolean native integer type.
    /// @param offset The first byte index.
    /// @param value The integer value.
    /// @param endianness The byte order.
    /// @throws err::OutOfRangeError If the integer does not fit at `offset`.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    void setIntegerOrThrow(
        const unit::ByteIndex offset, const T value, const Endianness endianness = Endianness::Little) {
        if (!setInteger(offset, value, endianness)) {
            impl::throwOutOfRange("Integer byte range out of range");
        }
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

public: // memory
    /// Detach shared storage.
    void detach();
    /// Get the current capacity.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength;
    /// Reserve capacity.
    void reserve(unit::ByteLength capacity);
    /// Shrink storage to the current length.
    void shrinkToFit();

public: // conversion
    /// Create a deep-copying byte buffer with the visible values.
    [[nodiscard]] auto toByteBuffer() const -> ByteBuffer;
    /// Create a vector with raw unsigned byte values.
    [[nodiscard]] auto toUInt8Vector() const -> std::vector<uint8_t>;
    /// Create a vector with raw char values.
    [[nodiscard]] auto toCharVector() const -> std::vector<char>;

public: // factory methods
    /// Create an editor by copying an Erbsland Core byte span.
    [[nodiscard]] static auto fromSpan(ConstByteSpan bytes) -> ByteBlockEditor;
    /// Create an editor by copying standard byte values.
    [[nodiscard]] static auto fromSpan(std::span<const std::byte> bytes) -> ByteBlockEditor;
    /// Create an editor by copying unsigned byte values.
    [[nodiscard]] static auto fromSpan(std::span<const uint8_t> bytes) -> ByteBlockEditor;
    /// Create an editor by copying character byte values.
    [[nodiscard]] static auto fromSpan(std::span<const char> bytes) -> ByteBlockEditor;
    /// Create an editor by copying unsigned byte values.
    [[nodiscard]] static auto fromVector(const std::vector<uint8_t> &bytes) -> ByteBlockEditor;
    /// Create an editor by copying character byte values.
    [[nodiscard]] static auto fromVector(const std::vector<char> &bytes) -> ByteBlockEditor;
    /// Join byte sequences without a separator.
    [[nodiscard]] static auto fromJoined(std::initializer_list<ByteBlock> parts) -> ByteBlockEditor;

private:
    /// Create a block from already prepared shared storage.
    explicit ByteBlockEditor(impl::ByteBlockDataPtr data) noexcept;
    /// Ensure non-null unique storage for the current size.
    void ensureUnique();
    /// Access the visible bytes through an internal borrowed view.
    [[nodiscard]] auto dataView() const noexcept -> impl::ByteDataView;
    /// Access the visible bytes for internal writes after detaching.
    [[nodiscard]] auto writableSpan() -> ByteSpan;
    /// Append zero-filled bytes and return the index of the first appended byte.
    [[nodiscard]] auto appendZeroed(unit::ByteLength length) -> unit::ByteIndex;

private:
    impl::ByteBlockDataPtr _data; ///< Shared byte storage.
};

}
