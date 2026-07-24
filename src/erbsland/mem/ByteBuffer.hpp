// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteBuffer_fwd.hpp"
#include "ByteIntegerAccess.hpp"
#include "ByteSpan.hpp"
#include "Endianness.hpp"

#include "impl/ByteBufferData.hpp"
#include "impl/ByteIntegerAccess.hpp"
#include "impl/ByteSequenceOperations.hpp"
#include "impl/Throw.hpp"
#include "impl/UnsafeByteBufferAccess_fwd.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/ByteRange.hpp"
#include "../util/impl/ComparisonHelper.hpp"
#include "../util/LoopResult.hpp"

#include <compare>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace erbsland::mem {

/// A dynamic, deep-copying buffer of explicit byte values.
/// The object uniquely owns a compact byte allocation and never shares storage through copy-on-write.
/// @seedoc{/reference/mem/byte_utilities}
/// @tested{ByteBufferTest}
class ByteBuffer final {
    friend class impl::UnsafeByteBufferAccess;

public:
    /// Create an empty buffer.
    ByteBuffer() = default;
    /// Create a buffer filled with a byte value.
    explicit ByteBuffer(unit::ByteLength length, Byte value = Byte{});
    /// Create a buffer by copying explicit byte values.
    ByteBuffer(std::initializer_list<Byte> bytes);
    /// Create a buffer by copying a borrowed byte span.
    explicit ByteBuffer(ConstByteSpan bytes);

    // defaults
    ~ByteBuffer();
    ByteBuffer(const ByteBuffer &other);
    ByteBuffer(ByteBuffer &&other) noexcept;
    auto operator=(const ByteBuffer &other) -> ByteBuffer &;
    auto operator=(ByteBuffer &&other) noexcept -> ByteBuffer &;

public: // comparison
    [[nodiscard]] auto operator<=>(const ByteBuffer &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const ByteBuffer &other, other);

public: // accessors
    /// Test if this buffer contains no bytes.
    [[nodiscard]] auto isEmpty() const noexcept -> bool;
    /// Get the visible byte length.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength;
    /// Get the index after the final byte.
    [[nodiscard]] auto endIndex() const noexcept -> unit::ByteIndex { return unit::ByteIndex::end(length()); }
    /// Get the allocated capacity.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength;
    /// Test if discarded storage is securely erased.
    [[nodiscard]] auto isSensitive() const noexcept -> bool { return _data.isSensitive(); }
    /// Enable or disable secure erasure for discarded storage.
    /// Disabling this mode securely erases the complete allocation and discards all visible bytes.
    void setSensitive(bool sensitive) noexcept;
    /// Access all bytes through a read-only borrowed span.
    [[nodiscard]] auto span() const noexcept -> ConstByteSpan;
    /// Access a clamped range through a read-only borrowed span.
    [[nodiscard]] auto span(unit::ByteRange range) const noexcept -> ConstByteSpan {
        return impl::clampedSpan(span(), range);
    }
    /// Access a clamped range through a read-only borrowed span.
    [[nodiscard]] auto span(unit::ByteIndex index, unit::ByteLength lengthValue) const noexcept -> ConstByteSpan {
        return span(unit::ByteRange{index, lengthValue});
    }
    /// Get a byte or a default value if its index is invalid.
    [[nodiscard]] auto get(unit::ByteIndex index, Byte defaultValue = Byte{}) const noexcept -> Byte;
    /// Get a byte or throw if its index is invalid.
    /// @throws err::OutOfRangeError If `index` is invalid or outside this buffer.
    [[nodiscard]] auto getOrThrow(unit::ByteIndex index) const -> Byte;
    /// Invoke a callback for every byte and its optional index.
    template <typename Function>
    auto forEach(Function function) const -> util::LoopResult {
        return impl::forEachByte(span(), std::move(function));
    }

public: // integers
    /// Read an integer or return a default value if its range is invalid.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] auto getInteger(
        const unit::ByteIndex offset,
        const Endianness endianness = Endianness::Little,
        const T defaultOnError = T{}) const noexcept -> T {
        return mem::getInteger<T>(span(), offset, endianness, defaultOnError);
    }
    /// Read an integer or throw if its range is invalid.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] auto getIntegerOrThrow(
        const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) const -> T {
        return mem::getIntegerOrThrow<T>(span(), offset, endianness);
    }
    /// Decode an integer into an existing value.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    auto getIntegerInto(
        T &value, const unit::ByteIndex offset, const Endianness endianness = Endianness::Little) const noexcept
        -> bool {
        return mem::getIntegerInto(span(), value, offset, endianness);
    }

public: // write
    /// Set a byte, ignoring invalid indexes.
    void set(unit::ByteIndex index, Byte value) noexcept;
    /// Set a byte or throw for an invalid index.
    void setOrThrow(unit::ByteIndex index, Byte value);
    /// XOR a byte value at an index, ignoring invalid indexes.
    void xorAt(unit::ByteIndex index, Byte value) noexcept;
    /// XOR a byte value at an index or throw if its index is invalid.
    /// @throws err::OutOfRangeError If `index` is invalid or outside this buffer.
    void xorAtOrThrow(unit::ByteIndex index, Byte value);
    /// Store an integer, leaving this buffer unchanged if its range is invalid.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    auto setInteger(
        const unit::ByteIndex offset, const T value, const Endianness endianness = Endianness::Little) noexcept
        -> bool {
        return mem::setInteger(writableSpan(), offset, value, endianness);
    }
    /// Store an integer or throw if its range is invalid.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    void setIntegerOrThrow(
        const unit::ByteIndex offset, const T value, const Endianness endianness = Endianness::Little) {
        mem::setIntegerOrThrow(writableSpan(), offset, value, endianness);
    }
    /// Fill all visible bytes.
    void fill(Byte value) noexcept { impl::fill(writableSpan(), unit::ByteRange::all(), value); }
    /// Fill a clamped byte range.
    void fill(unit::ByteRange targetRange, Byte value) noexcept { impl::fill(writableSpan(), targetRange, value); }
    /// Overwrite from the beginning with as many source bytes as fit.
    void overwrite(ConstByteSpan source);
    /// Overwrite from an index with as many source bytes as fit.
    void overwrite(unit::ByteIndex index, ConstByteSpan source);
    /// Overwrite a clamped target range with as many source bytes as fit.
    void overwrite(unit::ByteRange targetRange, ConstByteSpan source);
    /// XOR every byte with an equally sized source.
    /// @return `false` without changing the buffer if the lengths differ.
    auto xorWith(ConstByteSpan source) -> bool;
    /// XOR a clamped target range with as many source bytes as fit.
    void xorWith(unit::ByteRange targetRange, ConstByteSpan source);
    /// Securely erase the complete allocated capacity while preserving length and capacity.
    void secureErase() noexcept;

public: // storage
    /// Resize visible storage, zero-filling growth.
    auto resize(unit::ByteLength lengthValue) -> ByteBuffer &;
    /// Reserve at least the requested capacity.
    void reserve(unit::ByteLength capacityValue);
    /// Reduce capacity to the visible length.
    void shrinkToFit();
    /// Remove all visible bytes while preserving capacity.
    auto clear() noexcept -> ByteBuffer &;
    /// Remove all bytes and release storage.
    void reset() noexcept;
    /// Append a byte.
    auto append(Byte value) -> ByteBuffer &;
    /// Append borrowed bytes.
    auto append(ConstByteSpan bytes) -> ByteBuffer &;
    /// Insert borrowed bytes, clamping out-of-range indexes to the end.
    auto insert(unit::ByteIndex index, ConstByteSpan bytes) -> ByteBuffer &;
    /// Replace a clamped range with borrowed bytes.
    auto replace(unit::ByteRange range, ConstByteSpan replacement) -> ByteBuffer &;
    /// Remove a clamped range.
    auto remove(unit::ByteRange range) -> ByteBuffer &;
    /// Keep only a clamped range.
    auto keep(unit::ByteRange range) -> ByteBuffer &;

public: // conversion
    /// Copy into raw unsigned-byte storage.
    [[nodiscard]] auto toUInt8Vector() const -> std::vector<uint8_t>;
    /// Copy into raw character storage.
    [[nodiscard]] auto toCharVector() const -> std::vector<char>;

public: // factories
    /// Copy standard byte values.
    [[nodiscard]] static auto fromSpan(std::span<const std::byte> bytes) -> ByteBuffer;
    /// Copy raw unsigned-byte values.
    [[nodiscard]] static auto fromSpan(std::span<const uint8_t> bytes) -> ByteBuffer;
    /// Copy raw character values.
    [[nodiscard]] static auto fromSpan(std::span<const char> bytes) -> ByteBuffer;

private:
    [[nodiscard]] auto writableSpan() noexcept -> ByteSpan;
    void swap(ByteBuffer &other) noexcept;

private:
    impl::ByteBufferData _data; ///< The uniquely owned dynamic storage and its persistent mode flags.
};

}
