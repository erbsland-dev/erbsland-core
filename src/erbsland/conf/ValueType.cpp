// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueType.hpp"

#include "../text/StringEditor.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace erbsland::conf {

using namespace text::literals;

ValueType::ValueToTextMap ValueType::_valueToTextMap = {
    std::make_tuple(Undefined, "Undefined"_el),
    std::make_tuple(Integer, "Integer"_el),
    std::make_tuple(Boolean, "Boolean"_el),
    std::make_tuple(Float, "Float"_el),
    std::make_tuple(Text, "Text"_el),
    std::make_tuple(Date, "Date"_el),
    std::make_tuple(Time, "Time"_el),
    std::make_tuple(DateTime, "DateTime"_el),
    std::make_tuple(Bytes, "Bytes"_el),
    std::make_tuple(TimeDelta, "TimeDelta"_el),
    std::make_tuple(RegEx, "RegEx"_el),
    std::make_tuple(ValueList, "ValueList"_el),
    std::make_tuple(SectionList, "SectionList"_el),
    std::make_tuple(IntermediateSection, "IntermediateSection"_el),
    std::make_tuple(SectionWithNames, "SectionWithNames"_el),
    std::make_tuple(SectionWithTexts, "SectionWithTexts"_el),
    std::make_tuple(Document, "Document"_el),
};

auto ValueType::toText() const noexcept -> text::String {
    auto it = std::ranges::find_if(
        _valueToTextMap, [this](const ValueToTextEntry &entry) -> bool { return std::get<0>(entry) == _value; });
    assert(it != _valueToTextMap.end());
    return std::get<1>(*it);
}

auto ValueType::toValueDescription(const bool withArticle) const -> text::String {
    text::String result;
    switch (_value) {
    case Integer:
        result = "integer value"_el;
        break;
    case Boolean:
        result = "boolean value"_el;
        break;
    case Float:
        result = "floating-point value"_el;
        break;
    case Text:
        result = "text value"_el;
        break;
    case Date:
        result = "date value"_el;
        break;
    case Time:
        result = "time value"_el;
        break;
    case DateTime:
        result = "date-time value"_el;
        break;
    case Bytes:
        result = "bytes value"_el;
        break;
    case TimeDelta:
        result = "time-delta value"_el;
        break;
    case RegEx:
        result = "regular expression"_el;
        break;
    case ValueList:
        result = "value list"_el;
        break;
    case SectionList:
        result = "section list"_el;
        break;
    case IntermediateSection:
        result = "intermediate section"_el;
        break;
    case SectionWithNames:
        result = "section"_el;
        break;
    case SectionWithTexts:
        result = "section with texts"_el;
        break;
    case Document:
        result = "document"_el;
        break;
    default:
        break;
    }
    if (withArticle && !result.isEmpty()) {
        if (result.startsWith("i"_el)) {
            result = text::StringEditor::fromJoined({"an "_el, result});
        } else {
            result = text::StringEditor::fromJoined({"a "_el, result});
        }
    }
    return result;
}

auto ValueType::all() noexcept -> const std::array<ValueType, 17> & {
    static const std::array<ValueType, 17> values = {
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
        ValueList,
        SectionList,
        IntermediateSection,
        SectionWithNames,
        SectionWithTexts,
        Document};
    return values;
}

}
