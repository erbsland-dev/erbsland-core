// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "JsonWriter.hpp"

#include "../../EscapeFormat.hpp"
#include "../../Literals.hpp"

namespace erbsland::text::json::impl {

using namespace text::literals;

auto JsonWriter::write() -> String {
    writeValue(_value, unit::ItemCount{});
    return _result;
}

void JsonWriter::writeValue(const JsonValue &value, const unit::ItemCount depth) {
    switch (value.type()) {
    case JsonType::Null:
        _result.append("null"_el);
        break;
    case JsonType::Bool:
        _result.append(value.getBoolOrThrow() ? "true"_el : "false"_el);
        break;
    case JsonType::Number: {
        const auto integer = value.get<int64_t>();
        if (integer.has_value()) {
            _result.append(String::fromInteger(*integer));
        } else {
            _result.append(String::fromFloat(value.getNumberOrThrow()));
        }
        break;
    }
    case JsonType::Text:
        writeString(value.getTextOrThrow());
        break;
    case JsonType::Array:
        writeArray(value.getOrThrow<JsonArray>(), depth);
        break;
    case JsonType::Object:
        writeObject(value.getOrThrow<JsonObject>(), depth);
        break;
    }
}

void JsonWriter::writeArray(const JsonArray &array, const unit::ItemCount depth) {
    _result.append(U'[');
    if (array.isEmpty()) {
        _result.append(U']');
        return;
    }
    const auto childDepth = depth + unit::ItemCount::one();
    auto isFirst = true;
    for (const auto &entry : array) {
        if (!isFirst) {
            _result.append(U',');
        }
        if (isPretty()) {
            _result.append(U'\n');
            writeIndent(childDepth);
        }
        writeValue(entry, childDepth);
        isFirst = false;
    }
    if (isPretty()) {
        _result.append(U'\n');
        writeIndent(depth);
    }
    _result.append(U']');
}

void JsonWriter::writeObject(const JsonObject &object, const unit::ItemCount depth) {
    _result.append(U'{');
    if (object.count().isZero()) {
        _result.append(U'}');
        return;
    }
    const auto childDepth = depth + unit::ItemCount::one();
    auto isFirst = true;
    for (const auto &[key, entry] : object) {
        if (!isFirst) {
            _result.append(U',');
        }
        if (isPretty()) {
            _result.append(U'\n');
            writeIndent(childDepth);
        }
        writeString(key);
        _result.append(isPretty() ? ": "_el : ":"_el);
        writeValue(entry, childDepth);
        isFirst = false;
    }
    if (isPretty()) {
        _result.append(U'\n');
        writeIndent(depth);
    }
    _result.append(U'}');
}

void JsonWriter::writeString(const String &value) {
    _result.append(U'"');
    _result.append(value.toEscaped(EscapeFormat::Json, _options.escapeAmount()));
    _result.append(U'"');
}

void JsonWriter::writeIndent(const unit::ItemCount depth) {
    for (auto level = unit::ItemCount{}; level < depth; ++level) {
        _result.append(U' ', _options.indentation());
    }
}

auto JsonWriter::isPretty() const noexcept -> bool {
    return !_options.indentation().isZero();
}

}
