// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InputStream.hpp"

#include "../mem/Byte.hpp"
#include "../mem/ByteBlock.hpp"
#include "../mem/Endianness.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"

#include <concepts>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

namespace erbsland::stream {

class ByteInputStream;
using ByteInputStreamPtr = std::shared_ptr<ByteInputStream>;

/// A stream that reads raw bytes.
/// Byte input streams use `read` names instead of formatted extraction operators. Short reads are normal and report
/// the number of bytes actually read. End-of-stream is represented by a zero byte count or an empty optional.
/// Convenience methods are built on top of `read(std::span<mem::Byte>)`.
/// @tested{ByteStreamTest}
class ByteInputStream : public InputStream {
public:
    ~ByteInputStream() override = default;

public: // accessors
    /// Get the byte order used by integer convenience methods.
    [[nodiscard]] virtual auto endianness() const noexcept -> mem::Endianness;
    /// Set the byte order used by integer convenience methods.
    virtual void setEndianness(mem::Endianness endianness) noexcept;

public: // core interface
    /// Read bytes into a destination span.
    /// @param destination The destination bytes to fill.
    /// @return The number of bytes read. A zero length means end-of-stream if `destination` was not empty.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] virtual auto read(std::span<mem::Byte> destination) -> unit::ByteLength = 0;

public: // default interface
    /// Read up to `maximumLength` bytes and return them as a byte block.
    /// @param maximumLength The maximum number of bytes to read.
    /// @return The bytes that were read. An empty block means end-of-stream if `maximumLength` was not zero.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] virtual auto read(unit::ByteLength maximumLength) -> mem::ByteBlock;
    /// Read exactly `length` bytes.
    /// @param length The number of bytes to read.
    /// @return The bytes that were read, or an empty optional if end-of-stream is reached first.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] virtual auto readExact(unit::ByteLength length) -> std::optional<mem::ByteBlock>;
    /// Read exactly `length` bytes.
    /// @param length The number of bytes to read.
    /// @return The bytes that were read.
    /// @throws err::StreamError If the stream is closed, the backing source fails, or end-of-stream is reached first.
    [[nodiscard]] virtual auto readExactOrThrow(unit::ByteLength length) -> mem::ByteBlock;
    /// Read one byte.
    /// @return The next byte, or an empty optional at end-of-stream.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] virtual auto readByte() -> std::optional<mem::Byte>;
    /// Read one byte.
    /// @return The next byte.
    /// @throws err::StreamError If the stream is closed, the backing source fails, or end-of-stream is reached first.
    [[nodiscard]] virtual auto readByteOrThrow() -> mem::Byte;
    /// Read all remaining bytes.
    /// @return All bytes from the current position to the end of the stream.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] virtual auto readAll() -> mem::ByteBlock;

public: // integer read
    /// Read an integer value.
    /// @return The integer value, or an empty optional if end-of-stream is reached before all bytes are read.
    /// @throws err::StreamError If the stream is closed or the backing source fails.
    template <std::integral T>
    [[nodiscard]] auto readInteger() -> std::optional<T> {
        const auto data = readExact(unit::ByteLength::fromSizeT(sizeof(T)));
        if (!data.has_value()) {
            return std::nullopt;
        }
        auto result = std::make_unsigned_t<T>{0};
        for (auto i = std::size_t{0}; i < sizeof(T); ++i) {
            const auto byte = static_cast<std::make_unsigned_t<T>>(data->get(unit::ByteIndex::fromSizeT(i)).toUInt8());
            const auto shift = _endianness == mem::Endianness::Little ? i * 8U : (sizeof(T) - 1U - i) * 8U;
            result |= byte << shift;
        }
        return static_cast<T>(result);
    }
    /// Read an integer value.
    /// @return The integer value.
    /// @throws err::StreamError If the stream is closed, the backing source fails, or end-of-stream is reached first.
    template <std::integral T>
    [[nodiscard]] auto readIntegerOrThrow() -> T {
        const auto result = readInteger<T>();
        if (!result.has_value()) {
            throw err::StreamError{"Unexpected end of byte stream."};
        }
        return *result;
    }

public: // integer wrappers
    /// Read a signed 8-bit integer.
    [[nodiscard]] auto readInt8() -> std::optional<int8_t> { return readInteger<int8_t>(); }
    /// Read an unsigned 8-bit integer.
    [[nodiscard]] auto readUInt8() -> std::optional<uint8_t> { return readInteger<uint8_t>(); }
    /// Read a signed 16-bit integer.
    [[nodiscard]] auto readInt16() -> std::optional<int16_t> { return readInteger<int16_t>(); }
    /// Read an unsigned 16-bit integer.
    [[nodiscard]] auto readUInt16() -> std::optional<uint16_t> { return readInteger<uint16_t>(); }
    /// Read a signed 32-bit integer.
    [[nodiscard]] auto readInt32() -> std::optional<int32_t> { return readInteger<int32_t>(); }
    /// Read an unsigned 32-bit integer.
    [[nodiscard]] auto readUInt32() -> std::optional<uint32_t> { return readInteger<uint32_t>(); }
    /// Read a signed 64-bit integer.
    [[nodiscard]] auto readInt64() -> std::optional<int64_t> { return readInteger<int64_t>(); }
    /// Read an unsigned 64-bit integer.
    [[nodiscard]] auto readUInt64() -> std::optional<uint64_t> { return readInteger<uint64_t>(); }

    /// Read a signed 8-bit integer or throw on failure.
    [[nodiscard]] auto readInt8OrThrow() -> int8_t { return readIntegerOrThrow<int8_t>(); }
    /// Read an unsigned 8-bit integer or throw on failure.
    [[nodiscard]] auto readUInt8OrThrow() -> uint8_t { return readIntegerOrThrow<uint8_t>(); }
    /// Read a signed 16-bit integer or throw on failure.
    [[nodiscard]] auto readInt16OrThrow() -> int16_t { return readIntegerOrThrow<int16_t>(); }
    /// Read an unsigned 16-bit integer or throw on failure.
    [[nodiscard]] auto readUInt16OrThrow() -> uint16_t { return readIntegerOrThrow<uint16_t>(); }
    /// Read a signed 32-bit integer or throw on failure.
    [[nodiscard]] auto readInt32OrThrow() -> int32_t { return readIntegerOrThrow<int32_t>(); }
    /// Read an unsigned 32-bit integer or throw on failure.
    [[nodiscard]] auto readUInt32OrThrow() -> uint32_t { return readIntegerOrThrow<uint32_t>(); }
    /// Read a signed 64-bit integer or throw on failure.
    [[nodiscard]] auto readInt64OrThrow() -> int64_t { return readIntegerOrThrow<int64_t>(); }
    /// Read an unsigned 64-bit integer or throw on failure.
    [[nodiscard]] auto readUInt64OrThrow() -> uint64_t { return readIntegerOrThrow<uint64_t>(); }

private:
    mem::Endianness _endianness{mem::Endianness::Little}; ///< Integer byte order.
};

}
