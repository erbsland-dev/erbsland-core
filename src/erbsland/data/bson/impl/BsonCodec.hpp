// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BsonValue.hpp"

#include "../../../mem/ByteReader.hpp"
#include "../../../mem/ByteWriter.hpp"

namespace erbsland::data::bson::impl {
/// Internal BSON document parser and writer.
/// @tested{BsonValueTest}
class BsonCodec final {
public: // conversion
    /// Parse one complete BSON document within the requested limits.
    /// @param bytes The complete input bytes or byte sequence.
    /// @param options The parse or format options.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto decode(const mem::ByteBlock &bytes, BsonParseOptions options) -> BsonValue;
    /// Serialize a BSON document root to one wire frame.
    /// @param value The value to read, write, or inspect.
    /// @param options The parse or format options.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto encode(const BsonValue &value, BsonFormatOptions options) -> mem::ByteBlock;

private: // construction
    /// Create a codec for parsing or formatting.
    /// @param bytes The complete input bytes or byte sequence.
    /// @param options The parse or format options.
    explicit BsonCodec(mem::ByteBlock bytes, BsonParseOptions options);
    /// Create a codec for parsing or formatting.
    /// @param options The parse or format options.
    explicit BsonCodec([[maybe_unused]] BsonFormatOptions options);

private: // parsing
    /// Read the root document and require the end of input.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto parse() -> BsonValue;
    /// Read a BSON document or indexed array.
    /// @param array Whether the container uses indexed array keys.
    /// @param depth The current container nesting depth.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readContainer(bool array, unit::ItemCount depth) -> BsonValue;
    /// Decode a value for a BSON type code.
    /// @param typeCode The BSON wire type code.
    /// @param depth The current container nesting depth.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readValue(uint8_t typeCode, unit::ItemCount depth) -> BsonValue;
    /// Read a NUL-terminated BSON key.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readCString() -> text::String;
    /// Read a length-prefixed BSON string.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readString() -> text::String;
    /// Read UTC milliseconds into a Core date/time.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readDateTime() -> time::DateTime;
    /// Wrap wire bytes as a tolerant Core string.
    /// @param bytes The complete input bytes or byte sequence.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto bytesToText(const mem::ByteBlock &bytes) -> text::String;

private: // serialization
    /// Write a BSON document or indexed array.
    /// @param value The value to read, write, or inspect.
    /// @param array Whether the container uses indexed array keys.
    void writeContainer(const BsonValue &value, bool array);
    /// Write the payload of one BSON value.
    /// @param value The value to read, write, or inspect.
    void writeValue(const BsonValue &value);
    /// Write a NUL-terminated BSON key.
    /// @param value The value to read, write, or inspect.
    void writeCString(const text::String &value);
    /// Write a length-prefixed BSON string.
    /// @param value The value to read, write, or inspect.
    void writeString(const text::String &value);
    /// Write a millisecond-precision UTC timestamp.
    /// @param value The value to read, write, or inspect.
    void writeDateTime(const time::DateTime &value);
    /// Select the BSON type code for a value.
    /// @param value The value to read, write, or inspect.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto typeCode(const BsonValue &value) -> uint8_t;
    /// Report malformed or unsupported BSON input.
    /// @throws err::ParseError Always, for malformed or unsupported input.
    [[noreturn]] static void fail();

private:                            // data
    mem::ByteBlock _input;          ///< Original data for shared slices.
    mem::ByteReader _reader;        ///< Sole reader.
    mem::ByteWriter _writer;        ///< Sole writer.
    BsonParseOptions _parseOptions; ///< Parse limits.
    unit::ItemCount _valueCount;    ///< Parsed values.
};
}
