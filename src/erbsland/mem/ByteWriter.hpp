// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteBlock.hpp"
#include "Endianness.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace erbsland::mem {

/// A sequential byte writer that produces a `ByteBlock`.
/// @tested{ByteReaderWriterTest}
class ByteWriter final {
public:
    ByteWriter() = default;
    ~ByteWriter() = default;
    ByteWriter(const ByteWriter &) = default;
    ByteWriter(ByteWriter &&) = default;
    auto operator=(const ByteWriter &) -> ByteWriter & = default;
    auto operator=(ByteWriter &&) -> ByteWriter & = default;

public: // accessors
    /// Get the current byte length.
    [[nodiscard]] auto length() const noexcept -> unit::ByteLength { return _block.length(); }
    /// Get the current write position.
    [[nodiscard]] auto position() const noexcept -> unit::ByteIndex { return _position; }
    /// Set the write position, clamped to `length()`.
    void setPosition(unit::ByteIndex position) noexcept;
    /// Get the byte order for multi-byte integers.
    [[nodiscard]] auto endianness() const noexcept -> Endianness { return _endianness; }
    /// Set the byte order for multi-byte integers.
    void setEndianness(Endianness endianness) noexcept { _endianness = endianness; }
    /// Return the written bytes as a copy-on-write byte block.
    [[nodiscard]] auto toByteBlock() const noexcept -> ByteBlock { return _block; }

public: // write
    /// Reserve capacity for at least `capacity` bytes.
    /// @param capacity The minimum byte capacity to reserve.
    /// @return A reference to this writer.
    auto reserve(unit::ByteLength capacity) -> ByteWriter &;
    /// Write a byte at the current position and advance.
    auto writeByte(Byte value) -> ByteWriter &;
    /// Write an integer at the current position and advance.
    template <std::integral T>
    auto writeInteger(T value) -> ByteWriter & {
        using Unsigned = std::make_unsigned_t<T>;
        const auto unsignedValue = static_cast<Unsigned>(value);
        for (auto i = std::size_t{0}; i < sizeof(T); ++i) {
            const auto shift = _endianness == Endianness::Little ? i * 8U : (sizeof(T) - 1U - i) * 8U;
            writeByte(Byte{static_cast<uint8_t>((unsignedValue >> shift) & Unsigned{0xffU})});
        }
        return *this;
    }

public: // integer wrappers
    /// Write a signed 8-bit integer.
    auto writeInt8(int8_t value) -> ByteWriter & { return writeInteger(value); }
    /// Write an unsigned 8-bit integer.
    auto writeUInt8(uint8_t value) -> ByteWriter & { return writeInteger(value); }
    /// Write a signed 16-bit integer.
    auto writeInt16(int16_t value) -> ByteWriter & { return writeInteger(value); }
    /// Write an unsigned 16-bit integer.
    auto writeUInt16(uint16_t value) -> ByteWriter & { return writeInteger(value); }
    /// Write a signed 32-bit integer.
    auto writeInt32(int32_t value) -> ByteWriter & { return writeInteger(value); }
    /// Write an unsigned 32-bit integer.
    auto writeUInt32(uint32_t value) -> ByteWriter & { return writeInteger(value); }
    /// Write a signed 64-bit integer.
    auto writeInt64(int64_t value) -> ByteWriter & { return writeInteger(value); }
    /// Write an unsigned 64-bit integer.
    auto writeUInt64(uint64_t value) -> ByteWriter & { return writeInteger(value); }

private:
    ByteBlock _block;                           ///< The written bytes.
    unit::ByteIndex _position{};                ///< Current write position.
    Endianness _endianness{Endianness::Little}; ///< Integer byte order.
};

}
