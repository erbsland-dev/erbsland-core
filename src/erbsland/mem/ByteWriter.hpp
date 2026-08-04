// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Byte.hpp"
#include "ByteBlock.hpp"
#include "ByteBlockEditor.hpp"
#include "ByteIntegerFormat.hpp"
#include "ByteTextOptions.hpp"
#include "ByteWriter_fwd.hpp"
#include "Endianness.hpp"

#include "impl/ByteIntegerTypes.hpp"

#include "../text/String_fwd.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"

#include <cstdint>
#include <span>

namespace erbsland::mem {

/// A sequential byte writer that produces a `ByteBlock` or `ByteBlockEditor`.
/// It is the best choice to write multi-field binary data for protocols and byte based formats.
/// Make sure to set the correct endianness for multi-byte integers, default is `Endianness::Little`.
/// The various overloads for Erbsland Core types, allow to write compact binary representations
/// that can be read back with the corresponding `ByteReader` overloads.
/// @tested{ByteReaderWriterTest}
class ByteWriter final {
public:
    // defaults
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
    /// Reset the writer.
    /// This discards all written bytes and resets the write position.
    void reset() noexcept;
    /// Get the byte order for multi-byte integers.
    [[nodiscard]] auto endianness() const noexcept -> Endianness { return _endianness; }
    /// Set the byte order for multi-byte integers.
    void setEndianness(const Endianness endianness) noexcept { _endianness = endianness; }
    /// Return the written bytes as a byte block.
    [[nodiscard]] auto toByteBlock() const noexcept -> ByteBlock { return _block; }
    /// Take the written bytes as a byte block editor.
    /// This resets the write position of the writer.
    [[nodiscard]] auto takeByteBlockEditor() noexcept -> ByteBlockEditor;

public: // write
    /// Reserve capacity for at least `capacity` bytes.
    /// @param capacity The minimum byte capacity to reserve.
    /// @return A reference to this writer.
    auto reserve(unit::ByteLength capacity) -> ByteWriter &;
    /// Write a byte at the current position and advance.
    /// @param value The byte value.
    /// @return A reference to this writer.
    auto writeByte(Byte value) -> ByteWriter &;
    /// Write a block of data at the current position and advance.
    /// @param data The data to write.
    auto writeBytes(const ConstByteSpan &data) -> ByteWriter &;
    /// @overload
    auto writeBytes(const std::span<const std::byte> &data) -> ByteWriter &;
    /// @overload
    auto writeBytes(const ByteBlock &data) -> ByteWriter &;
    /// Write an integer at the current position and advance.
    /// @tparam T One of the fixed-width native integer types from `int8_t` through `uint64_t`.
    /// @param value The integer value.
    /// @return A reference to this writer.
    template <impl::NativeByteInteger T>
    auto writeInteger(T value) -> ByteWriter &;
    /// Write an integer using an explicit wire format.
    /// @tparam T One of the fixed-width native integer types from `int8_t` through `uint64_t`.
    /// @param value The integer value.
    /// @param format The integer wire format.
    /// @return A reference to this writer.
    /// @throws err::OutOfRangeError If the value cannot be represented by `format`.
    template <impl::NativeByteInteger T>
    auto writeIntegerOrThrow(T value, ByteIntegerFormat format) -> ByteWriter &;
    /// Write text, truncating it at character boundaries when necessary.
    /// @param text The text to write.
    /// @param options The encoding and framing options.
    /// @return A reference to this writer.
    /// @throws err::OutOfRangeError If even an empty text frame cannot fit.
    auto writeText(const text::String &text, const ByteTextOptions &options = {}) -> ByteWriter &;
    /// Write text using exactly the framing specified by `options`.
    /// @param text The text to write.
    /// @param options The encoding and framing options.
    /// @return A reference to this writer.
    /// @throws err::OutOfRangeError If the text cannot fit in the specified framing.
    auto writeTextOrThrow(const text::String &text, const ByteTextOptions &options = {}) -> ByteWriter &;

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
    ByteBlockEditor _block;                     ///< The written bytes.
    unit::ByteIndex _position;                  ///< Current write position.
    Endianness _endianness{Endianness::Little}; ///< Integer byte order.
};

}

#include "ByteWriter.tpp"
