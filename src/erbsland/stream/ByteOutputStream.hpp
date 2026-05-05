// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OutputStream.hpp"

#include "../mem/Byte.hpp"
#include "../mem/ByteBlockView.hpp"
#include "../mem/Endianness.hpp"

#include <array>
#include <concepts>
#include <cstdint>
#include <span>
#include <type_traits>

namespace erbsland::stream {

class ByteOutputStream;
using ByteOutputStreamPtr = std::shared_ptr<ByteOutputStream>;

/// A stream that writes raw bytes.
/// Byte output streams use explicit `write` methods. A successful call accepts the complete byte sequence; short
/// writes are handled internally or reported as `err::StreamError`. Convenience methods are built on top of
/// `write(std::span<const mem::Byte>)`.
/// @tested{ByteStreamTest}
class ByteOutputStream : public OutputStream {
public:
    ~ByteOutputStream() override = default;

public: // accessors
    /// Get the byte order used by integer convenience methods.
    [[nodiscard]] virtual auto endianness() const noexcept -> mem::Endianness;
    /// Set the byte order used by integer convenience methods.
    virtual void setEndianness(mem::Endianness endianness) noexcept;

public: // default interface
    /// Write a single byte.
    /// @param byte The byte to write.
    /// @throws err::StreamError If the stream is closed or the backing target fails.
    virtual void write(mem::Byte byte);

public: // core interface
    /// Write a byte span.
    /// @param bytes The bytes to write.
    /// @throws err::StreamError If the stream is closed or the backing target fails.
    virtual void write(std::span<const mem::Byte> bytes) = 0;

public: // default interface
    /// Write a byte block view.
    /// @param bytes The bytes to write.
    /// @throws err::StreamError If the stream is closed or the backing target fails.
    virtual void write(const mem::ByteBlockView &bytes);

public: // integer write
    /// Write an integer value.
    /// @param value The integer value.
    /// @throws err::StreamError If the stream is closed or the backing target fails.
    template <std::integral T>
    void writeInteger(T value) {
        using Unsigned = std::make_unsigned_t<T>;
        const auto unsignedValue = static_cast<Unsigned>(value);
        auto bytes = std::array<mem::Byte, sizeof(T)>{};
        for (auto i = std::size_t{0}; i < sizeof(T); ++i) {
            const auto shift = _endianness == mem::Endianness::Little ? i * 8U : (sizeof(T) - 1U - i) * 8U;
            bytes[i] = mem::Byte{static_cast<uint8_t>((unsignedValue >> shift) & Unsigned{0xffU})};
        }
        write(std::span<const mem::Byte>{bytes});
    }

public: // integer wrappers
    /// Write a signed 8-bit integer.
    void writeInt8(int8_t value) { writeInteger(value); }
    /// Write an unsigned 8-bit integer.
    void writeUInt8(uint8_t value) { writeInteger(value); }
    /// Write a signed 16-bit integer.
    void writeInt16(int16_t value) { writeInteger(value); }
    /// Write an unsigned 16-bit integer.
    void writeUInt16(uint16_t value) { writeInteger(value); }
    /// Write a signed 32-bit integer.
    void writeInt32(int32_t value) { writeInteger(value); }
    /// Write an unsigned 32-bit integer.
    void writeUInt32(uint32_t value) { writeInteger(value); }
    /// Write a signed 64-bit integer.
    void writeInt64(int64_t value) { writeInteger(value); }
    /// Write an unsigned 64-bit integer.
    void writeUInt64(uint64_t value) { writeInteger(value); }

private:
    mem::Endianness _endianness{mem::Endianness::Little}; ///< Integer byte order.
};

}
