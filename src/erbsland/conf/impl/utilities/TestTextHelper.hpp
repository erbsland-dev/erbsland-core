// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TypeTraits.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../../re/RegEx.hpp"
#include "../../../text/ByteFormat.hpp"
#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../TestFormat.hpp"
#include "../../Value.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

template <typename T>
[[nodiscard]] auto toTestText(const T &, TestFormat) noexcept -> text::String {
    static_assert(always_false_v<T>, "Not implemented for the given type.");
    return {};
}

template <typename T>
    requires(std::is_same_v<T, bool>)
[[nodiscard]] auto toTestText(const T &value, TestFormat) noexcept -> text::String {
    return value ? "true"_el : "false"_el;
}

template <typename T>
    requires(!std::is_same_v<T, bool> && (std::is_integral_v<T> || std::is_floating_point_v<T>))
[[nodiscard]] auto toTestText(const T &value, TestFormat) noexcept -> text::String {
    return text::StringFormat{"{}"_el}.build(value);
}

template <>
[[nodiscard]] inline auto toTestText(const text::String &value, TestFormat) noexcept -> text::String {
    return text::StringFormat{"\"{:/config_test}\""_el}.build(value);
}

template <>
[[nodiscard]] inline auto toTestText(const time::Date &value, TestFormat) noexcept -> text::String {
    return value.toString();
}

template <>
[[nodiscard]] inline auto toTestText(const time::Time &value, TestFormat) noexcept -> text::String {
    return value.toString();
}

template <>
[[nodiscard]] inline auto toTestText(const time::TimeWithZone &value, TestFormat) noexcept -> text::String {
    if (!value.timeZone().isLocalTime() && value.timeZone().isUtc()) {
        return text::String::fromJoined({value.time().toString(), "z"_el});
    }
    return value.toString();
}

template <>
[[nodiscard]] inline auto toTestText(const time::DateTime &value, TestFormat) noexcept -> text::String {
    if (!value.isLocalTime() && value.isUtc()) {
        return text::String::fromJoined({value.date().toString(), " "_el, value.time().toString(), "z"_el});
    }
    return value.toString();
}

template <>
[[nodiscard]] inline auto toTestText(const mem::ByteBlock &value, TestFormat) noexcept -> text::String {
    return text::String::fromByteBlock(value, text::ByteFormat::compact());
}

template <>
[[nodiscard]] inline auto toTestText(const re::RegExPtr &value, TestFormat) noexcept -> text::String {
    return text::StringFormat{"\"{:/config_test}\""_el}.build(value->pattern().toString());
}

template <>
[[nodiscard]] inline auto toTestText(const time::CalendarDelta &value, TestFormat) noexcept -> text::String {
    return value.toString(time::TimeDeltaFormat::elcl());
}

template <>
[[nodiscard]] inline auto toTestText(const conf::Value &value, const TestFormat format) noexcept -> text::String {
    text::String valueText;
    switch (value.type().raw()) {
    case ValueType::Integer:
        valueText = toTestText(value.asInteger(), format);
        break;
    case ValueType::Boolean:
        valueText = toTestText(value.asBoolean(), format);
        break;
    case ValueType::Float:
        valueText = toTestText(value.asFloat(), format);
        break;
    case ValueType::Text:
        valueText = toTestText(value.asText(), format);
        break;
    case ValueType::Date:
        valueText = toTestText(value.asDate(), format);
        break;
    case ValueType::Time:
        valueText = toTestText(value.asTimeWithZone(), format);
        break;
    case ValueType::DateTime:
        valueText = toTestText(value.asDateTime(), format);
        break;
    case ValueType::Bytes:
        valueText = toTestText(value.asBytes(), format);
        break;
    case ValueType::TimeDelta:
        valueText = toTestText(value.asCalendarDelta(), format);
        break;
    case ValueType::RegEx:
        valueText = toTestText(value.asRegEx(), format);
        break;
    case ValueType::ValueList:
    case ValueType::SectionList:
    case ValueType::IntermediateSection:
    case ValueType::SectionWithNames:
    case ValueType::SectionWithTexts:
    case ValueType::Document:
        if (format.isSet(TestFormat::ShowContainerSize)) {
            valueText = text::StringFormat{"size={}"_el}.build(value.size());
        }
        break;
    case ValueType::Undefined:
        return "Undefined()"_el;
    }
    return text::StringFormat{"{}({})"_el}.build(value.type().toText(), valueText);
}

}
