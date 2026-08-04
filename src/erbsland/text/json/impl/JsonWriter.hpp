// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../JsonFormatOptions.hpp"
#include "../JsonValue.hpp"

#include "../../StringEditor.hpp"

namespace erbsland::text::json::impl {

/// Serialize a JSON value tree.
/// @tested{JsonParserTest}
class JsonWriter final {
public:
    /// Create a writer for one JSON value tree.
    JsonWriter(const JsonValue &value, JsonFormatOptions options) : _value{value}, _options{options} {}
    /// Serialize the configured JSON value.
    [[nodiscard]] auto write() -> String;

private:
    /// Write one value at the given nesting depth.
    void writeValue(const JsonValue &value, unit::ItemCount depth);
    /// Write an array.
    void writeArray(const JsonArray &array, unit::ItemCount depth);
    /// Write an object.
    void writeObject(const JsonObject &object, unit::ItemCount depth);
    /// Write one escaped JSON string.
    void writeString(const String &value);
    /// Write indentation for a line.
    void writeIndent(unit::ItemCount depth);
    /// Test whether pretty output is enabled.
    [[nodiscard]] auto isPretty() const noexcept -> bool;

private:
    const JsonValue &_value;    ///< Root value.
    JsonFormatOptions _options; ///< Formatting options.
    StringEditor _result;       ///< Output buffer.
};

}
