// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RuleType.hpp"

#include <cassert>

namespace erbsland::conf::vr {

using namespace text::literals;

auto RuleType::all() noexcept -> const std::array<RuleType, 19> & {
    static const std::array<RuleType, 19> values = {
        Undefined,
        Integer,
        Boolean,
        Float,
        Text,
        Date,
        Time,
        DateTime,
        Bytes,
        TimeDelta,
        RegEx,
        Value,
        ValueList,
        ValueMatrix,
        Section,
        SectionList,
        SectionWithTexts,
        NotValidated,
        Alternatives};
    return values;
}

auto RuleType::valueToTextMap() noexcept -> const ValueMap & {
    static const ValueMap valueMap = {
        Entry{Undefined, "Undefined"_el, ValueType::Undefined, text::String{}},
        Entry{Integer, "Integer"_el, ValueType::Integer, "an integer value"_el},
        Entry{Boolean, "Boolean"_el, ValueType::Boolean, "a Boolean value"_el},
        Entry{Float, "Float"_el, ValueType::Float, "a floating-point or integer value"_el},
        Entry{Text, "Text"_el, ValueType::Text, "a text value"_el},
        Entry{Date, "Date"_el, ValueType::Date, "a date value"_el},
        Entry{Time, "Time"_el, ValueType::Time, "a time value"_el},
        Entry{DateTime, "DateTime"_el, ValueType::DateTime, "a date-time value"_el},
        Entry{Bytes, "Bytes"_el, ValueType::Bytes, "a byte value"_el},
        Entry{TimeDelta, "TimeDelta"_el, ValueType::TimeDelta, "a time-delta value"_el},
        Entry{RegEx, "RegEx"_el, ValueType::RegEx, "a regular expression"_el},
        Entry{Value, "Value"_el, ValueType::Undefined, "any scalar value"_el},
        Entry{ValueList, "ValueList"_el, ValueType::ValueList, "a value list or scalar value"_el},
        Entry{ValueMatrix, "ValueMatrix"_el, ValueType::Undefined, "a nested value list or scalar value"_el},
        Entry{Section, "Section"_el, ValueType::SectionWithNames, "a section"_el},
        Entry{SectionList, "SectionList"_el, ValueType::SectionList, "a section list"_el},
        Entry{SectionWithTexts, "SectionWithTexts"_el, ValueType::SectionWithTexts, "a section with texts"_el},
        Entry{NotValidated, "NotValidated"_el, ValueType::Undefined, text::String{}},
        Entry{Alternatives, "Alternatives"_el, ValueType::Undefined, text::String{}},
    };
    return valueMap;
}

auto RuleType::textToValueMap() noexcept -> const TextToValueMap & {
    static const TextToValueMap textToValueMap = {
        {"integer"_el, Integer},
        {"boolean"_el, Boolean},
        {"float"_el, Float},
        {"text"_el, Text},
        {"date"_el, Date},
        {"time"_el, Time},
        {"datetime"_el, DateTime},
        {"date_time"_el, DateTime},
        {"bytes"_el, Bytes},
        {"timedelta"_el, TimeDelta},
        {"time_delta"_el, TimeDelta},
        {"regex"_el, RegEx},
        {"value"_el, Value},
        {"valuelist"_el, ValueList},
        {"value_list"_el, ValueList},
        {"valuematrix"_el, ValueMatrix},
        {"value_matrix"_el, ValueMatrix},
        {"section"_el, Section},
        {"sectionwithnames"_el, Section},
        {"section_with_names"_el, Section},
        {"sectionlist"_el, SectionList},
        {"section_list"_el, SectionList},
        {"sectionwithtexts"_el, SectionWithTexts},
        {"section_with_texts"_el, SectionWithTexts},
        {"notvalidated"_el, NotValidated},
        {"not_validated"_el, NotValidated},
    };
    return textToValueMap;
}

auto RuleType::matchesValueType(const ValueType valueType) const noexcept -> bool {
    switch (_value) {
    case Undefined:
        return false; // coverage: undefined rules should not exist.
    case Value:
        return valueType.isScalar();
    case ValueList:
    case ValueMatrix:
        return valueType == ValueType::ValueList || valueType.isScalar();
    case NotValidated: // coverage: should be handled with a custom logic.
    case Alternatives: // coverage: should be handled with a custom logic.
        return true;
    case Section:
        return valueType == ValueType::SectionWithNames || valueType == ValueType::IntermediateSection;
    default:
        return valueType == toValueType();
    }
}

auto RuleType::toText() const noexcept -> const text::String & {
    return entry().text;
}

auto RuleType::fromText(const text::String &text) noexcept -> RuleType {
    if (text.isEmpty() || text.length().toSizeT() > 20) {
        return Undefined;
    }
    const auto &map = textToValueMap();
    for (const auto &[name, value] : map) {
        if (name.compare(text, text::Char::compareIdentifier) == std::strong_ordering::equal) {
            return value;
        }
    }
    return Undefined;
}

auto RuleType::toValueType() const noexcept -> ValueType {
    return entry().valueType;
}

auto RuleType::expectedValueTypeText() const noexcept -> const text::String & {
    return entry().expectedValueTypeText;
}

auto RuleType::entry() const noexcept -> const Entry & {
    const auto &map = valueToTextMap();
    const auto it = std::ranges::find_if(map, [this](const Entry &entry) -> bool { return entry.value == _value; });
    assert(it != map.end());
    return *it;
}

}
