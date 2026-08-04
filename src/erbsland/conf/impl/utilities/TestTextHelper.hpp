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

/// Convert a supported configuration test value to its textual representation.
/// @tparam T The value type.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <typename T>
[[nodiscard]] auto toTestText([[maybe_unused]] const T &value, [[maybe_unused]] TestFormat format) noexcept
    -> text::String {
    static_assert(always_false_v<T>, "Not implemented for the given type.");
    return {};
}

/// Convert a Boolean configuration test value to text.
/// @tparam T The Boolean value type.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <typename T>
    requires(std::is_same_v<T, bool>)
[[nodiscard]] auto toTestText(const T &value, [[maybe_unused]] TestFormat format) noexcept -> text::String {
    return value ? "true"_el : "false"_el;
}

/// Convert a numeric configuration test value to text.
/// @tparam T The integral or floating-point value type.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <typename T>
    requires(!std::is_same_v<T, bool> && (std::is_integral_v<T> || std::is_floating_point_v<T>))
[[nodiscard]] auto toTestText(const T &value, [[maybe_unused]] TestFormat format) noexcept -> text::String {
    return text::StringFormat{"{}"_el}.build(value);
}

/// Convert a text configuration test value to quoted ELCL text.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <>
[[nodiscard]] inline auto toTestText(const text::String &value, [[maybe_unused]] TestFormat format) noexcept
    -> text::String {
    return text::StringFormat{"\"{:/config_test}\""_el}.build(value);
}

/// Convert a date configuration test value to text.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <>
[[nodiscard]] inline auto toTestText(const time::Date &value, [[maybe_unused]] TestFormat format) noexcept
    -> text::String {
    return value.toString();
}

/// Convert a time configuration test value to text.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <>
[[nodiscard]] inline auto toTestText(const time::Time &value, [[maybe_unused]] TestFormat format) noexcept
    -> text::String {
    return value.toString();
}

/// Convert a time-with-zone configuration test value to text.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <>
[[nodiscard]] inline auto toTestText(const time::TimeWithZone &value, [[maybe_unused]] TestFormat format) noexcept
    -> text::String {
    if (!value.timeZone().isLocalTime() && value.timeZone().isUtc()) {
        return text::String::fromJoined({value.time().toString(), "z"_el});
    }
    return value.toString();
}

/// Convert a date-time configuration test value to text.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <>
[[nodiscard]] inline auto toTestText(const time::DateTime &value, [[maybe_unused]] TestFormat format) noexcept
    -> text::String {
    if (!value.isLocalTime() && value.isUtc()) {
        return text::String::fromJoined({value.date().toString(), " "_el, value.time().toString(), "z"_el});
    }
    return value.toString();
}

/// Convert a byte-block configuration test value to compact text.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <>
[[nodiscard]] inline auto toTestText(const mem::ByteBlock &value, [[maybe_unused]] TestFormat format) noexcept
    -> text::String {
    return text::String::fromByteBlock(value, text::ByteFormat::compact());
}

/// Convert a regular-expression configuration test value to quoted text.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <>
[[nodiscard]] inline auto toTestText(const re::RegExPtr &value, [[maybe_unused]] TestFormat format) noexcept
    -> text::String {
    return text::StringFormat{"\"{:/config_test}\""_el}.build(value->pattern().toString());
}

/// Convert a calendar-delta configuration test value to text.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
template <>
[[nodiscard]] inline auto toTestText(const time::CalendarDelta &value, [[maybe_unused]] TestFormat format) noexcept
    -> text::String {
    return value.toString(time::TimeDeltaFormat::elcl());
}

/// Convert a generic configuration value to test text.
/// @param value The value to convert.
/// @param format The selected test-text options.
/// @return The formatted test text.
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
