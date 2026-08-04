// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteBlock.hpp"
#include "ByteBlockEditor.hpp"
#include "ByteIntegerFormat.hpp"
#include "ByteReader_fwd.hpp"
#include "ByteTextOptions.hpp"
#include "Endianness.hpp"

#include "impl/ByteDataView.hpp"
#include "impl/ByteIntegerTypes.hpp"
#include "impl/ByteReaderTools_fwd.hpp"

#include "../text/String_fwd.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace erbsland::mem {

/// A sequential byte reader for read-only or editable byte blocks.
/// @tested{ByteReaderWriterTest}
class ByteReader final {
    friend class impl::ByteReaderTools;

public:
    /// Create a reader sharing the data from a byte block editor.
    ByteReader(const ByteBlockEditor &editor) noexcept; // NOLINT(*-explicit-constructor)
    /// Create a reader sharing the data from a read-only byte block.
    ByteReader(const ByteBlock &block) noexcept; // NOLINT(*-explicit-constructor)

    // defaults
    ByteReader() = default;
    ~ByteReader() = default;
    ByteReader(const ByteReader &) = default;
    ByteReader(ByteReader &&) = default;
    auto operator=(const ByteReader &) -> ByteReader & = default;
    auto operator=(ByteReader &&) -> ByteReader & = default;

public: // accessors
    /// Get the readable byte length.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength;
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
    [[nodiscard]] auto canRead(unit::ByteLength byteCount) const noexcept -> bool;
    /// Compatibility overload using a native byte count.
    [[nodiscard]] auto canRead(std::size_t byteCount) const noexcept -> bool {
        return canRead(unit::ByteLength::fromSizeT(byteCount));
    }
    /// Advance the current position by `byteCount`, clamped to `length()`.
    void advance(unit::ByteLength byteCount) noexcept;
    /// Compatibility overload using a native byte count.
    void advance(std::size_t byteCount) noexcept { advance(unit::ByteLength::fromSizeT(byteCount)); }

public: // byte read
    /// Read one byte or return zero at end.
    [[nodiscard]] auto readByte() noexcept -> Byte;
    /// Read one byte without advancing, or return zero at end.
    [[nodiscard]] auto peekByte() const noexcept -> Byte;
    /// Read one byte at an absolute position without advancing, or return a default value.
    [[nodiscard]] auto peekByte(unit::ByteIndex index, Byte defaultValue = Byte{}) const noexcept -> Byte;
    /// Read one byte at an offset from the current position without advancing, or return a default value.
    [[nodiscard]] auto peekByte(std::size_t offset, Byte defaultValue = Byte{}) const noexcept -> Byte;
    /// Read one byte or throw at end.
    /// @throws err::OutOfRangeError If there is no byte at the current position.
    [[nodiscard]] auto readByteOrThrow() -> Byte;
    /// Read one byte without advancing, or throw at end.
    /// @throws err::OutOfRangeError If there is no byte at the current position.
    [[nodiscard]] auto peekByteOrThrow() const -> Byte;
    /// Read exactly `length` bytes into a byte block.
    /// Returns no value without advancing if there are not enough bytes.
    /// @param length The number of bytes to read.
    [[nodiscard]] auto readBytes(unit::ByteLength length) noexcept -> std::optional<ByteBlock>;
    /// Read exactly `length` bytes into a byte block.
    /// @param length The number of bytes to read.
    /// @return A shared block containing the requested bytes.
    /// @throws err::OutOfRangeError If there are not enough bytes.
    [[nodiscard]] auto readBytesOrThrow(unit::ByteLength length) -> ByteBlock;
    /// Read text using the given framing, or no value if it is incomplete or invalid.
    /// @param options The encoding and framing options.
    /// @return The decoded text, or no value if the complete valid frame is unavailable.
    [[nodiscard]] auto readText(const ByteTextOptions &options = {}) -> std::optional<text::String>;
    /// Read text using the given framing.
    /// @param options The encoding and framing options.
    /// @return The decoded text.
    /// @throws err::OutOfRangeError If the complete field is unavailable.
    /// @throws err::ParseError If an end mark is missing or mismatched.
    [[nodiscard]] auto readTextOrThrow(const ByteTextOptions &options = {}) -> text::String;

public: // integer read
    /// Read an integer value or return `defaultOnError` if there are not enough bytes.
    /// @tparam T One of the fixed-width native integer types from `int8_t` through `uint64_t`.
    /// @param defaultOnError The value returned when the complete integer is unavailable.
    /// @return The decoded integer, or `defaultOnError`.
    template <impl::NativeByteInteger T>
    [[nodiscard]] auto readInteger(T defaultOnError = T{0}) noexcept -> T;
    /// Read an integer value or throw if there are not enough bytes.
    /// @tparam T One of the fixed-width native integer types from `int8_t` through `uint64_t`.
    /// @return The decoded integer.
    /// @throws err::OutOfRangeError If there are not enough bytes.
    template <impl::NativeByteInteger T>
    [[nodiscard]] auto readIntegerOrThrow() -> T;
    /// Read an explicitly formatted integer, or no value if it is incomplete or does not fit `T`.
    /// @tparam T One of the fixed-width native integer types from `int8_t` through `uint64_t`.
    /// @param format The integer wire format.
    /// @return The decoded integer, or no value if decoding fails.
    template <impl::NativeByteInteger T>
    [[nodiscard]] auto readInteger(ByteIntegerFormat format) -> std::optional<T>;
    /// @overload
    template <impl::NativeByteInteger T>
    [[nodiscard]] auto readInteger(ByteIntegerFormat::Value format) -> std::optional<T>;
    /// Read an explicitly formatted integer.
    /// @tparam T One of the fixed-width native integer types from `int8_t` through `uint64_t`.
    /// @param format The integer wire format.
    /// @return The decoded integer.
    /// @throws err::OutOfRangeError If the complete wire value is unavailable.
    /// @throws err::OverflowError If the wire value cannot fit `T`.
    template <impl::NativeByteInteger T>
    [[nodiscard]] auto readIntegerOrThrow(ByteIntegerFormat format) -> T;
    /// @overload
    template <impl::NativeByteInteger T>
    [[nodiscard]] auto readIntegerOrThrow(ByteIntegerFormat::Value format) -> T;
    /// Read an integer into `value`.
    /// @tparam T One of the fixed-width native integer types from `int8_t` through `uint64_t`.
    /// @param value The destination, which remains unchanged if the complete integer is unavailable.
    /// @return `true` on success, `false` when there are not enough bytes.
    template <impl::NativeByteInteger T>
    [[nodiscard]] auto readIntegerInto(T &value) noexcept -> bool;

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
    /// Access the complete reader input through an internal borrowed view.
    [[nodiscard]] auto dataView() const noexcept -> impl::ByteDataView;

private:
    ByteBlock _block;                           ///< The shared readable bytes.
    unit::ByteIndex _position{};                ///< Current read position.
    Endianness _endianness{Endianness::Little}; ///< Integer byte order.
};

}

#include "ByteReader.tpp"
