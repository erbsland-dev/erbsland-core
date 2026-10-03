// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TokenKind.hpp"

namespace erbsland::conf::syntax {

auto TokenKind::toValueType() const noexcept -> ValueType {
    switch (_value) {
    case TokenKind::Integer:
        return ValueType::Integer;
    case TokenKind::Float:
        return ValueType::Float;
    case TokenKind::Boolean:
        return ValueType::Boolean;
    case TokenKind::Text:
    case TokenKind::Code:
    case TokenKind::MultiLineTextOpen:
    case TokenKind::MultiLineCodeOpen:
        return ValueType::Text;
    case TokenKind::RegEx:
    case TokenKind::MultiLineRegexOpen:
        return ValueType::RegEx;
    case TokenKind::Bytes:
    case TokenKind::MultiLineBytesOpen:
        return ValueType::Bytes;
    case TokenKind::Date:
        return ValueType::Date;
    case TokenKind::Time:
        return ValueType::Time;
    case TokenKind::DateTime:
        return ValueType::DateTime;
    case TokenKind::TimeDelta:
        return ValueType::TimeDelta;
    default:
        return ValueType::Undefined;
    }
}

auto TokenKind::isPayload() const noexcept -> bool {
    return _value == TokenKind::MultiLineText || _value == TokenKind::MultiLineCode ||
        _value == TokenKind::MultiLineRegex || _value == TokenKind::MultiLineBytes;
}

auto TokenKind::isValue() const noexcept -> bool {
    switch (_value) {
    case TokenKind::Integer:
    case TokenKind::Boolean:
    case TokenKind::Float:
    case TokenKind::Text:
    case TokenKind::Code:
    case TokenKind::RegEx:
    case TokenKind::Bytes:
    case TokenKind::Date:
    case TokenKind::Time:
    case TokenKind::DateTime:
    case TokenKind::TimeDelta:
        return true;
    default:
        return false;
    }
}

auto TokenKind::toText() const noexcept -> text::String {
    using namespace text::literals;
    switch (_value) {
    case TokenKind::EndOfData:
        return "EndOfData"_el;
    case TokenKind::LineBreak:
        return "LineBreak"_el;
    case TokenKind::Spacing:
        return "Spacing"_el;
    case TokenKind::Indentation:
        return "Indentation"_el;
    case TokenKind::Comment:
        return "Comment"_el;
    case TokenKind::RegularName:
        return "RegularName"_el;
    case TokenKind::TextName:
        return "TextName"_el;
    case TokenKind::MetaName:
        return "MetaName"_el;
    case TokenKind::NameValueSeparator:
        return "NameValueSeparator"_el;
    case TokenKind::ValueListSeparator:
        return "ValueListSeparator"_el;
    case TokenKind::MultiLineValueListSeparator:
        return "MultiLineValueListSeparator"_el;
    case TokenKind::NamePathSeparator:
        return "NamePathSeparator"_el;
    case TokenKind::Integer:
        return "Integer"_el;
    case TokenKind::Boolean:
        return "Boolean"_el;
    case TokenKind::Float:
        return "Float"_el;
    case TokenKind::Text:
        return "Text"_el;
    case TokenKind::MultiLineTextOpen:
        return "MultiLineTextOpen"_el;
    case TokenKind::MultiLineTextClose:
        return "MultiLineTextClose"_el;
    case TokenKind::MultiLineText:
        return "MultiLineText"_el;
    case TokenKind::Code:
        return "Code"_el;
    case TokenKind::MultiLineCodeOpen:
        return "MultiLineCodeOpen"_el;
    case TokenKind::MultiLineCodeLanguage:
        return "MultiLineCodeLanguage"_el;
    case TokenKind::MultiLineCodeClose:
        return "MultiLineCodeClose"_el;
    case TokenKind::MultiLineCode:
        return "MultiLineCode"_el;
    case TokenKind::RegEx:
        return "RegEx"_el;
    case TokenKind::MultiLineRegexOpen:
        return "MultiLineRegexOpen"_el;
    case TokenKind::MultiLineRegexClose:
        return "MultiLineRegexClose"_el;
    case TokenKind::MultiLineRegex:
        return "MultiLineRegex"_el;
    case TokenKind::Bytes:
        return "Bytes"_el;
    case TokenKind::MultiLineBytesOpen:
        return "MultiLineBytesOpen"_el;
    case TokenKind::MultiLineBytesFormat:
        return "MultiLineBytesFormat"_el;
    case TokenKind::MultiLineBytesClose:
        return "MultiLineBytesClose"_el;
    case TokenKind::MultiLineBytes:
        return "MultiLineBytes"_el;
    case TokenKind::Date:
        return "Date"_el;
    case TokenKind::Time:
        return "Time"_el;
    case TokenKind::DateTime:
        return "DateTime"_el;
    case TokenKind::TimeDelta:
        return "TimeDelta"_el;
    case TokenKind::SectionMapOpen:
        return "SectionMapOpen"_el;
    case TokenKind::SectionMapClose:
        return "SectionMapClose"_el;
    case TokenKind::SectionListOpen:
        return "SectionListOpen"_el;
    case TokenKind::SectionListClose:
        return "SectionListClose"_el;
    case TokenKind::SectionName:
        return "SectionName"_el;
    case TokenKind::SectionTextName:
        return "SectionTextName"_el;
    case TokenKind::Error:
        return "Error"_el;
    }
    return {};
}

}
