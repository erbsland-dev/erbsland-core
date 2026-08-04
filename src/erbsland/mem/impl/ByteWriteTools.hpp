// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteDataView.hpp"
#include "Throw.hpp"

#include "../ByteIntegerAccess.hpp"
#include "../Endianness.hpp"

#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"

#include <concepts>
#include <type_traits>

namespace erbsland::mem::impl {

/// Implements fixed-size mutations on a writable byte span.
/// @tested{ByteDataViewTest ByteArrayTest ByteBufferTest ByteBlockTest}
class ByteWriteTools final {
public:
    /// Create tools for the given writable bytes.
    explicit constexpr ByteWriteTools(ByteSpan data) noexcept : _data{data} {}

public: // indexed write
    /// Set a byte, ignoring an invalid index.
    constexpr void set(const unit::ByteIndex index, const Byte value) noexcept {
        if (index.isValid() && index.toSizeT() < _data.size()) {
            _data[index.toSizeT()] = value;
        }
    }
    /// Set a byte or throw for an invalid index.
    constexpr void setOrThrow(const unit::ByteIndex index, const Byte value) {
        if (!index.isValid() || index.toSizeT() >= _data.size()) {
            throwOutOfRange("Byte write position out of range");
        }
        _data[index.toSizeT()] = value;
    }
    /// XOR a byte, ignoring an invalid index.
    constexpr void xorAt(const unit::ByteIndex index, const Byte value) noexcept {
        if (index.isValid() && index.toSizeT() < _data.size()) {
            _data[index.toSizeT()] ^= value;
        }
    }
    /// XOR a byte or throw for an invalid index.
    constexpr void xorAtOrThrow(const unit::ByteIndex index, const Byte value) {
        if (!index.isValid() || index.toSizeT() >= _data.size()) {
            throwOutOfRange("Byte write position out of range");
        }
        _data[index.toSizeT()] ^= value;
    }

public: // integer write
    /// Store a native integer, leaving the bytes unchanged for an invalid range.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    [[nodiscard]] constexpr auto setInteger(
        const unit::ByteIndex index, const T value, const Endianness endianness = Endianness::Little) noexcept -> bool {
        return mem::setInteger(_data, index, value, endianness);
    }
    /// Store a native integer or throw for an invalid range.
    template <typename T>
        requires(std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>)
    constexpr void setIntegerOrThrow(
        const unit::ByteIndex index, const T value, const Endianness endianness = Endianness::Little) {
        mem::setIntegerOrThrow(_data, index, value, endianness);
    }

public: // bulk write
    /// Fill a clamped byte range.
    void fill(unit::ByteRange range, Byte value) noexcept;
    /// Overwrite the largest possible part of a clamped byte range.
    void overwrite(unit::ByteRange range, ByteDataView source) noexcept;
    /// XOR the largest possible part of a clamped byte range.
    void xorWith(unit::ByteRange range, ByteDataView source) noexcept;

private:
    /// Access a clamped writable byte range.
    [[nodiscard]] auto span(unit::ByteRange range) const noexcept -> ByteSpan;

private:
    ByteSpan _data; ///< The borrowed writable data operated on by these tools.
};

}
