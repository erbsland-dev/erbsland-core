// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteBlock.hpp"
#include "ByteBlockEditor.hpp"
#include "Endianness.hpp"

#include "impl/Throw.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace erbsland::mem {

/// A sequential byte reader for read-only or editable byte blocks.
/// @tested{ByteReaderWriterTest}
class ByteReader final {
public:
    /// Create a reader sharing the data from a byte block editor.
    ByteReader(const ByteBlockEditor &editor) noexcept; // NOLINT(*-explicit-constructor)
    /// Create a reader sharing the data from a read-only byte block.
    ByteReader(const ByteBlock &block) noexcept; // NOLINT(*-explicit-constructor)

    ByteReader() = default;
    ~ByteReader() = default;
    ByteReader(const ByteReader &) = default;
    ByteReader(ByteReader &&) = default;
    auto operator=(const ByteReader &) -> ByteReader & = default;
    auto operator=(ByteReader &&) -> ByteReader & = default;

public: // accessors
    /// Get the readable byte length.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength { return _block.length(); }
    /// Get the current read position.
    [[nodiscard]] auto position() const noexcept -> unit::ByteIndex { return _position; }
    /// Set the read position, clamped to `length()`.
    void setPosition(unit::ByteIndex position) noexcept;
    /// Get the byte order for multi-byte integers.
    [[nodiscard]] auto endianness() const noexcept -> Endianness { return _endianness; }
    /// Set the byte order for multi-byte integers.
    void setEndianness(Endianness endianness) noexcept { _endianness = endianness; }
    /// Test if the reader is at the end of the data.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool { return _position >= unit::ByteIndex::end(length()); }
    /// Test if `byteCount` bytes can be read at the current position.
    [[nodiscard]] auto canRead(std::size_t byteCount) const noexcept -> bool;
    /// Advance the current position by `byteCount`, clamped to `length()`.
    void advance(std::size_t byteCount) noexcept;

public: // byte read
    /// Read one byte or return zero at end.
    [[nodiscard]] auto readByte() noexcept -> Byte;
    /// Read one byte without advancing, or return zero at end.
    [[nodiscard]] auto peekByte() const noexcept -> Byte;
    /// Read one byte at an offset from the current position without advancing, or return a default value.
    [[nodiscard]] auto peekByte(std::size_t offset, Byte defaultValue = Byte{}) const noexcept -> Byte;
    /// Read one byte or throw at end.
    /// @throws err::OutOfRangeError If there is no byte at the current position.
    [[nodiscard]] auto readByteOrThrow() -> Byte;
    /// Read one byte without advancing, or throw at end.
    /// @throws err::OutOfRangeError If there is no byte at the current position.
    [[nodiscard]] auto peekByteOrThrow() const -> Byte;

public: // integer read
    /// Read an integer value or return `defaultOnError` if there are not enough bytes.
    template <std::integral T>
    [[nodiscard]] auto readInteger(T defaultOnError = T{0}) noexcept -> T {
        auto result = T{};
        if (!readIntegerInto(result)) {
            return defaultOnError;
        }
        return result;
    }
    /// Read an integer value or throw if there are not enough bytes.
    /// @throws err::OutOfRangeError If there are not enough bytes.
    template <std::integral T>
    [[nodiscard]] auto readIntegerOrThrow() -> T {
        if (!canRead(sizeof(T))) {
            impl::throwOutOfRange("Read position out of range");
        }
        auto result = readIntegerUnchecked<T>();
        advance(sizeof(T));
        return result;
    }
    /// Read an integer into `value`.
    /// @return `true` on success, `false` when there are not enough bytes.
    template <std::integral T>
    [[nodiscard]] auto readIntegerInto(T &value) noexcept -> bool {
        if (!canRead(sizeof(T))) {
            return false;
        }
        value = readIntegerUnchecked<T>();
        advance(sizeof(T));
        return true;
    }

public: // integer wrappers
    /// Read a signed 8-bit integer or return `defaultOnError`.
    [[nodiscard]] auto readInt8(int8_t defaultOnError = 0) noexcept -> int8_t { return readInteger(defaultOnError); }
    /// Read an unsigned 8-bit integer or return `defaultOnError`.
    [[nodiscard]] auto readUInt8(uint8_t defaultOnError = 0U) noexcept -> uint8_t {
        return readInteger(defaultOnError);
    }
    /// Read a signed 16-bit integer or return `defaultOnError`.
    [[nodiscard]] auto readInt16(int16_t defaultOnError = 0) noexcept -> int16_t { return readInteger(defaultOnError); }
    /// Read an unsigned 16-bit integer or return `defaultOnError`.
    [[nodiscard]] auto readUInt16(uint16_t defaultOnError = 0U) noexcept -> uint16_t {
        return readInteger(defaultOnError);
    }
    /// Read a signed 32-bit integer or return `defaultOnError`.
    [[nodiscard]] auto readInt32(int32_t defaultOnError = 0) noexcept -> int32_t { return readInteger(defaultOnError); }
    /// Read an unsigned 32-bit integer or return `defaultOnError`.
    [[nodiscard]] auto readUInt32(uint32_t defaultOnError = 0U) noexcept -> uint32_t {
        return readInteger(defaultOnError);
    }
    /// Read a signed 64-bit integer or return `defaultOnError`.
    [[nodiscard]] auto readInt64(int64_t defaultOnError = 0) noexcept -> int64_t { return readInteger(defaultOnError); }
    /// Read an unsigned 64-bit integer or return `defaultOnError`.
    [[nodiscard]] auto readUInt64(uint64_t defaultOnError = 0U) noexcept -> uint64_t {
        return readInteger(defaultOnError);
    }

    /// Read a signed 8-bit integer or throw if there are not enough bytes.
    [[nodiscard]] auto readInt8OrThrow() -> int8_t { return readIntegerOrThrow<int8_t>(); }
    /// Read an unsigned 8-bit integer or throw if there are not enough bytes.
    [[nodiscard]] auto readUInt8OrThrow() -> uint8_t { return readIntegerOrThrow<uint8_t>(); }
    /// Read a signed 16-bit integer or throw if there are not enough bytes.
    [[nodiscard]] auto readInt16OrThrow() -> int16_t { return readIntegerOrThrow<int16_t>(); }
    /// Read an unsigned 16-bit integer or throw if there are not enough bytes.
    [[nodiscard]] auto readUInt16OrThrow() -> uint16_t { return readIntegerOrThrow<uint16_t>(); }
    /// Read a signed 32-bit integer or throw if there are not enough bytes.
    [[nodiscard]] auto readInt32OrThrow() -> int32_t { return readIntegerOrThrow<int32_t>(); }
    /// Read an unsigned 32-bit integer or throw if there are not enough bytes.
    [[nodiscard]] auto readUInt32OrThrow() -> uint32_t { return readIntegerOrThrow<uint32_t>(); }
    /// Read a signed 64-bit integer or throw if there are not enough bytes.
    [[nodiscard]] auto readInt64OrThrow() -> int64_t { return readIntegerOrThrow<int64_t>(); }
    /// Read an unsigned 64-bit integer or throw if there are not enough bytes.
    [[nodiscard]] auto readUInt64OrThrow() -> uint64_t { return readIntegerOrThrow<uint64_t>(); }

private:
    /// Read an integer without bounds checks or position updates.
    template <std::integral T>
    [[nodiscard]] auto readIntegerUnchecked() const noexcept -> T {
        using Unsigned = std::make_unsigned_t<T>;
        auto result = Unsigned{0};
        for (auto i = std::size_t{0}; i < sizeof(T); ++i) {
            const auto byte =
                static_cast<Unsigned>(_block.get(_position + unit::ByteLength::fromSizeT(i)).toRawValue());
            const auto shift = _endianness == Endianness::Little ? i * 8U : (sizeof(T) - 1U - i) * 8U;
            result |= static_cast<Unsigned>(byte) << shift;
        }
        return static_cast<T>(result);
    }

private:
    ByteBlock _block;                           ///< The shared readable bytes.
    unit::ByteIndex _position{};                ///< Current read position.
    Endianness _endianness{Endianness::Little}; ///< Integer byte order.
};

}
