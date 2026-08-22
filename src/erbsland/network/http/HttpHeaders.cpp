// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpHeaders.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text;
using namespace text::literals;

HttpHeaders::HttpHeaders(HttpFieldList fields, const HttpHeaderLimits limits) :
    _fields{std::move(fields)}, _limits{limits} {
    validate(_fields);
}

auto HttpHeaders::serializedLength() const noexcept -> unit::ByteLength {
    auto result = unit::ByteLength{};
    for (const auto &fieldValue : _fields) {
        result += fieldValue.name().text().length();
        result += fieldValue.value().length();
        result += unit::ByteLength{4U};
    }
    return result;
}

auto HttpHeaders::hasField(const HttpFieldName &name) const noexcept -> bool {
    return name.isValid() &&
        _fields.anyOf([&name](const HttpField &fieldValue) -> bool { return matches(fieldValue, name); });
}

auto HttpHeaders::hasField(const HttpFieldType type) const noexcept -> bool {
    return type.isKnown() && hasField(HttpFieldName{type});
}

auto HttpHeaders::hasField(const String &name) const noexcept -> bool {
    const auto parsed = HttpFieldName::fromString(name);
    return parsed.isValid() && hasField(parsed);
}

auto HttpHeaders::getFirst(const HttpFieldName &name) const noexcept -> String {
    if (!name.isValid()) {
        return {};
    }
    const auto index =
        _fields.findFirstIf([&name](const HttpField &fieldValue) -> bool { return matches(fieldValue, name); });
    return index.isNoIndex() ? String{} : _fields.getRef(index).value();
}

auto HttpHeaders::getFirst(const HttpFieldType type) const noexcept -> String {
    return type.isKnown() ? getFirst(HttpFieldName{type}) : String{};
}

auto HttpHeaders::getFirst(const String &name) const noexcept -> String {
    const auto parsed = HttpFieldName::fromString(name);
    return parsed.isValid() ? getFirst(parsed) : String{};
}

auto HttpHeaders::getAll(const HttpFieldName &name) const -> StringList {
    auto result = StringList{};
    if (!name.isValid()) {
        return result;
    }
    for (const auto &fieldValue : _fields) {
        if (matches(fieldValue, name)) {
            result.append(fieldValue.value());
        }
    }
    return result;
}

auto HttpHeaders::getAll(const HttpFieldType type) const -> StringList {
    return type.isKnown() ? getAll(HttpFieldName{type}) : StringList{};
}

auto HttpHeaders::getAll(const String &name) const -> StringList {
    const auto parsed = HttpFieldName::fromString(name);
    return parsed.isValid() ? getAll(parsed) : StringList{};
}

auto HttpHeaders::addField(HttpField fieldValue) -> HttpHeaders & {
    auto result = _fields;
    result.append(std::move(fieldValue));
    validate(result);
    _fields = std::move(result);
    return *this;
}

auto HttpHeaders::addField(const HttpFieldType type, String value) -> HttpHeaders & {
    if (!type.isKnown()) {
        throw err::ParameterError{"A recognized field type is required."_el, "type"_el};
    }
    return addField(HttpField{HttpFieldName{type}, std::move(value)});
}

auto HttpHeaders::addField(String name, String value) -> HttpHeaders & {
    return addField(HttpField{std::move(name), std::move(value)});
}

auto HttpHeaders::setField(HttpField fieldValue) -> HttpHeaders & {
    auto result = HttpFieldList{};
    result.reserve(_fields.count() + unit::ItemCount::one());
    auto replaced = false;
    for (const auto &existing : _fields) {
        if (matches(existing, fieldValue.name())) {
            if (!replaced) {
                result.append(fieldValue);
                replaced = true;
            }
            continue;
        }
        result.append(existing);
    }
    if (!replaced) {
        result.append(std::move(fieldValue));
    }
    validate(result);
    _fields = std::move(result);
    return *this;
}

auto HttpHeaders::setField(const HttpFieldType type, String value) -> HttpHeaders & {
    if (!type.isKnown()) {
        throw err::ParameterError{"A recognized field type is required."_el, "type"_el};
    }
    return setField(HttpField{HttpFieldName{type}, std::move(value)});
}

auto HttpHeaders::setField(String name, String value) -> HttpHeaders & {
    return setField(HttpField{std::move(name), std::move(value)});
}

auto HttpHeaders::removeAllFields(const HttpFieldName &name) -> unit::ItemCount {
    if (!name.isValid()) {
        return {};
    }
    const auto previous = _fields.count();
    _fields.removeIf([&name](const HttpField &fieldValue) -> bool { return matches(fieldValue, name); });
    return previous - _fields.count();
}

auto HttpHeaders::removeAllFields(const HttpFieldType type) -> unit::ItemCount {
    return type.isKnown() ? removeAllFields(HttpFieldName{type}) : unit::ItemCount{};
}

auto HttpHeaders::removeAllFields(const String &name) -> unit::ItemCount {
    const auto parsed = HttpFieldName::fromString(name);
    return parsed.isValid() ? removeAllFields(parsed) : unit::ItemCount{};
}

auto HttpHeaders::contentType() const noexcept -> std::optional<HttpMediaType> {
    const auto text = getFirst(HttpFieldType::ContentType);
    if (text.isEmpty() && !hasField(HttpFieldType::ContentType)) {
        return std::nullopt;
    }
    const auto result = HttpMediaType::fromString(text);
    return result.isValid() ? std::optional{result} : std::nullopt;
}

auto HttpHeaders::setContentType(const HttpMediaType &mediaType) -> HttpHeaders & {
    if (!mediaType.isValid()) {
        throw err::ParameterError{"A valid media type is required."_el, "mediaType"_el};
    }
    return setField(HttpFieldType::ContentType, mediaType.toString());
}

auto HttpHeaders::matches(const HttpField &fieldValue, const HttpFieldName &name) noexcept -> bool {
    return fieldValue.name() == name;
}

void HttpHeaders::validate(const HttpFieldList &fields) const {
    if (fields.count() > _limits.maximumFieldCount()) {
        throw err::ParameterError{"The HTTP header exceeds the configured field-count limit."_el, "fields"_el};
    }
    auto aggregate = unit::ByteLength{};
    for (const auto &fieldValue : fields) {
        if (!fieldValue.isValid()) {
            throw err::ParameterError{"The HTTP header contains an invalid field."_el, "fields"_el};
        }
        if (fieldValue.name().text().length() > _limits.maximumNameLength()) {
            throw err::ParameterError{"An HTTP field name exceeds the configured length limit."_el, "fields"_el};
        }
        if (fieldValue.value().length() > _limits.maximumValueLength()) {
            throw err::ParameterError{"An HTTP field value exceeds the configured length limit."_el, "fields"_el};
        }
        aggregate += fieldValue.name().text().length();
        aggregate += fieldValue.value().length();
        aggregate += unit::ByteLength{4U};
        if (aggregate > _limits.maximumAggregateLength()) {
            throw err::ParameterError{"The HTTP header exceeds the configured aggregate length limit."_el, "fields"_el};
        }
    }
}

}
