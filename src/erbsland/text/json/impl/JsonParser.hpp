// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../JsonParseOptions.hpp"
#include "../JsonValue.hpp"

#include "../../StringCharReader.hpp"

namespace erbsland::text::json::impl {

/// Strict parser for one JSON value.
/// @tested{JsonParserTest}
class JsonParser final {
public:
    /// Create a parser for one JSON document.
    JsonParser(String text, JsonParseOptions options);
    /// Parse the configured JSON document.
    [[nodiscard]] auto parse() -> JsonValue;

private:
    /// Parse one value at the given container nesting level.
    [[nodiscard]] auto parseValue(unit::ItemCount nesting) -> JsonValue;
    /// Parse one array.
    [[nodiscard]] auto parseArray(unit::ItemCount nesting) -> JsonValue;
    /// Parse one object.
    [[nodiscard]] auto parseObject(unit::ItemCount nesting) -> JsonValue;
    /// Parse and decode one JSON string.
    [[nodiscard]] auto parseString() -> String;
    /// Parse and decode an escape sequence.
    void parseEscapeSequence();
    /// Parse one JSON number.
    [[nodiscard]] auto parseNumber() -> JsonValue;
    /// Skip RFC 8259 whitespace.
    void skipWhitespace();
    /// Account for one value and enforce the configured limit.
    void countValue();
    /// Throw a parse error at the current input position.
    [[noreturn]] void fail(const String &reason) const;

private:
    String _text;                ///< Complete source text.
    JsonParseOptions _options;   ///< Active safety limits.
    StringCharReader _reader;    ///< Sequential source reader.
    unit::ItemCount _valueCount; ///< Parsed value count.
};

}
