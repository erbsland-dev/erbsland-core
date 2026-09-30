// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CborValue.hpp"

#include "../../../mem/ByteReader.hpp"
#include "../../../mem/ByteWriter.hpp"

namespace erbsland::data::cbor::impl {
/// Internal bounded CBOR parser and writer.
/// @tested{CborValueTest}
class CborCodec final {
public: // conversion
    /// Parse one complete CBOR item within the requested limits.
    /// @param bytes The complete input bytes or byte sequence.
    /// @param options The parse or format options.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto decode(const mem::ByteBlock &bytes, CborParseOptions options) -> CborValue;
    /// Serialize one CBOR item in the selected profile.
    /// @param value The value to read, write, or inspect.
    /// @param options The parse or format options.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto encode(const CborValue &value, CborFormatOptions options) -> mem::ByteBlock;

private: // construction
    /// Create a codec for parsing or formatting.
    /// @param bytes The complete input bytes or byte sequence.
    /// @param options The parse or format options.
    explicit CborCodec(mem::ByteBlock bytes, CborParseOptions options);
    /// Create a codec for parsing or formatting.
    /// @param options The parse or format options.
    explicit CborCodec(CborFormatOptions options);

private: // parsing
    /// Read one top-level item and require the end of input.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto parse() -> CborValue;
    /// Decode one CBOR item at the current nesting depth.
    /// @param depth The current container nesting depth.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readValue(unit::ItemCount depth) -> CborValue;
    /// Read the additional integer or indefinite marker.
    /// @param additional The CBOR additional-information field.
    /// @param allowIndefinite Whether an indefinite marker is permitted.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readArgument(uint8_t additional, bool allowIndefinite = false) -> uint64_t;
    /// Read a definite or ordinary indefinite byte sequence.
    /// @param additional The CBOR additional-information field.
    /// @param major The CBOR major type.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readBytes(uint8_t additional, uint8_t major) -> mem::ByteBlock;
    /// Read a bounded CBOR text string.
    /// @param additional The CBOR additional-information field.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readText(uint8_t additional) -> text::String;
    /// Read a CBOR array.
    /// @param additional The CBOR additional-information field.
    /// @param depth The current container nesting depth.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readArray(uint8_t additional, unit::ItemCount depth) -> CborValue;
    /// Read a string-keyed CBOR map.
    /// @param additional The CBOR additional-information field.
    /// @param depth The current container nesting depth.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readMap(uint8_t additional, unit::ItemCount depth) -> CborValue;
    /// Read a supported date/time or CID tag.
    /// @param additional The CBOR additional-information field.
    /// @param depth The current container nesting depth.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readTag(uint8_t additional, unit::ItemCount depth) -> CborValue;
    /// Read a supported simple value or floating-point number.
    /// @param additional The CBOR additional-information field.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readSimple(uint8_t additional) -> CborValue;
    /// Expand an IEEE 754 binary16 bit pattern.
    /// @param bits The binary16 bit pattern.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto halfToDouble(uint16_t bits) noexcept -> double;
    /// Wrap byte-string content as tolerant Core text.
    /// @param bytes The complete input bytes or byte sequence.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto bytesToText(const mem::ByteBlock &bytes) -> text::String;
    /// Validate raw CIDv0 or CIDv1 framing and digest length.
    /// @param bytes The complete input bytes or byte sequence.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto validCid(const mem::ByteBlock &bytes) noexcept -> bool;

private: // serialization
    /// Write one CBOR item recursively.
    /// @param value The value to read, write, or inspect.
    void writeValue(const CborValue &value);
    /// Write a major type and shortest argument.
    /// @param major The CBOR major type.
    /// @param value The value to read, write, or inspect.
    void writeHead(uint8_t major, uint64_t value);
    /// Write one CBOR text string.
    /// @param value The value to read, write, or inspect.
    void writeText(const text::String &value);
    /// Write one CBOR byte string.
    /// @param value The value to read, write, or inspect.
    void writeBytes(const mem::ByteBlock &value);
    /// Report malformed or unsupported CBOR input.
    /// @throws err::ParseError Always, for malformed or unsupported input.
    [[noreturn]] static void fail();

private:                              // data
    mem::ByteBlock _input;            ///< Original block for zero-copy slices.
    mem::ByteReader _reader;          ///< Sole input reader.
    mem::ByteWriter _writer;          ///< Sole output writer and parse scratch buffer.
    CborParseOptions _parseOptions;   ///< Parse limits/profile.
    CborFormatOptions _formatOptions; ///< Output profile.
    unit::ItemCount _valueCount;      ///< Number of decoded values.
};
}
