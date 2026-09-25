// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/json/JsonValue.hpp>

namespace demo {

using el::json::JsonArray;
using el::json::JsonFormatOptions;
using el::json::JsonObject;
using el::json::JsonParseOptions;
using el::json::JsonValue;

/// Parse one complete JSON value and choose how to handle malformed input.
///
/// `fromString()` returns an optional value for ordinary validation, while `fromStringOrThrow()` reports a
/// `ParseError` when a document must be valid. Both functions reject trailing content and duplicate object keys.
void parseDocument() {
    const auto source = u8R"({"patch":"Yankı","voices":2,"enabled":true})"_el;
    if (const auto value = JsonValue::fromString(source)) {
        el::io::printLine("Patch: "_el, value->getOrThrow("patch"_el).getTextOrThrow());
    }

    // A throwing parse is useful at a boundary where invalid input needs a diagnostic.
    try {
        const auto value = JsonValue::fromStringOrThrow(R"({"voices":2,})"_el);
        el::io::printLine(value.toString());
    } catch (const el::err::ParseError &) {
        el::io::printLine("Invalid JSON document"_el);
    }
}

/// Follow object keys and array indexes through a parsed JSON tree.
///
/// The nonthrowing `get()` returns JSON null for a missing child. Use `getOrThrow()` and typed access when a field is
/// required, and check `type()` or an optional typed result when the input schema allows alternatives.
void inspectDocument() {
    const auto document = JsonValue::fromStringOrThrow(
        u8R"({"patch":"Yankı","oscillators":[{"wave":"sine"},{"wave":"triangle"}],"enabled":true})"_el);
    const auto oscillators = document.getOrThrow("oscillators"_el);
    const auto second = oscillators.getOrThrow(el::ItemIndex{1U});
    el::io::printLine("Wave: "_el, second.getOrThrow("wave"_el).getTextOrThrow());
    el::io::printLine("Enabled: "_el, document.getOrThrow("enabled"_el).getBoolOrThrow());
    el::io::printLine("Missing is null: "_el, document.get("gain"_el).is(el::json::JsonType::Null));
}

/// Build a JSON document from values, objects, and arrays.
///
/// Object keys are inserted with `set()`, array elements with `append()`, and a fetched child is a value. To change
/// a nested child, mutate that value and then set it back into its parent.
void buildDocument() {
    auto patch = JsonValue{JsonObject{}};
    patch.set("name"_el, el::String{u8"Yankı"_el});
    patch.set("active"_el, true);
    auto voices = JsonValue{JsonArray{}};
    voices.append(el::String{"sine"_el}).append(el::String{"triangle"_el});
    patch.set("voices"_el, voices);
    el::io::printLine(patch.toString());

    // Copies share values until changed; the original document still has two voices.
    auto revised = patch;
    auto moreVoices = revised.getOrThrow("voices"_el);
    moreVoices.append(el::String{"square"_el});
    revised.set("voices"_el, moreVoices);
    el::io::printLine("Original voices: "_el, patch.getOrThrow("voices"_el).itemCount().toSizeT());
    el::io::printLine("Revised voices: "_el, revised.getOrThrow("voices"_el).itemCount().toSizeT());
}

/// Select compact or indented JSON and control string escaping.
///
/// `JsonFormatOptions` controls output layout and escaping. The default is compact output with required JSON escapes;
/// `pretty()` starts with two spaces per nesting level. Non-ASCII escaping is useful for ASCII-only destinations.
void formatDocument() {
    const auto patch = JsonValue::fromStringOrThrow(u8R"({"patch":"Yankı","voices":["sine","triangle"]})"_el);
    el::io::printLine("Compact: "_el, patch.toString());
    el::io::printLine("Pretty:"_el);
    el::io::printLine(patch.toString(JsonFormatOptions::pretty()));
    el::io::printLine("Four-space array:"_el);
    el::io::printLine(
        JsonValue{JsonArray{JsonValue{2}}}.toString(JsonFormatOptions{}.setIndentation(el::CpLength{4U})));
    const auto ascii = JsonFormatOptions::compact().setEscapeAmount(el::EscapeAmount::NonAscii);
    el::io::printLine("ASCII: "_el, patch.toString(ascii));
}

/// Bound untrusted JSON input by source size, nesting, value count, and decoded string length.
///
/// `JsonParseOptions` measures the source in bytes, container depth in levels, values as a count including the root,
/// and decoded key or string length in Unicode code points. A document exceeding any limit fails parsing.
void limitDocument() {
    const auto input = JsonParseOptions{}.setMaximumInputLength(el::ByteLength{3U});
    const auto nesting = JsonParseOptions{}.setMaximumNesting(el::ItemCount{1U});
    const auto values = JsonParseOptions{}.setMaximumValueCount(el::ItemCount{2U});
    const auto strings = JsonParseOptions{}.setMaximumStringLength(el::CpLength{2U});
    el::io::printLine("Input limit accepts null: "_el, JsonValue::fromString("null"_el, input).has_value());
    el::io::printLine("Nesting limit accepts [[1]]: "_el, JsonValue::fromString("[[1]]"_el, nesting).has_value());
    el::io::printLine("Value limit accepts [1,2]: "_el, JsonValue::fromString("[1,2]"_el, values).has_value());
    el::io::printLine("String limit accepts abc: "_el, JsonValue::fromString("\"abc\""_el, strings).has_value());
    el::io::printLine("String limit accepts é: "_el, JsonValue::fromString(u8"\"é\""_el, strings).has_value());
}

}
