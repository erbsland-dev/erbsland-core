// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/conf/syntax/TokenKind.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(SyntaxTokenKind)
class SyntaxTokenKindTest final : public el::UnitTest {
private:
    using TokenKind = el::conf::syntax::TokenKind;
    using ValueType = el::conf::ValueType;

public:
    void testDelimiterFactories() {
        static_assert(TokenKind::fromMultiLineOpen(U'"') == TokenKind::MultiLineTextOpen);
        static_assert(TokenKind::fromMultiLineOpen(U'`') == TokenKind::MultiLineCodeOpen);
        static_assert(TokenKind::fromMultiLineOpen(U'/') == TokenKind::MultiLineRegexOpen);
        static_assert(TokenKind::fromMultiLineOpen(U'<') == TokenKind::MultiLineBytesOpen);
        static_assert(TokenKind::fromMultiLineClose(U'"') == TokenKind::MultiLineTextClose);
        static_assert(TokenKind::fromMultiLineClose(U'`') == TokenKind::MultiLineCodeClose);
        static_assert(TokenKind::fromMultiLineClose(U'/') == TokenKind::MultiLineRegexClose);
        static_assert(TokenKind::fromMultiLineClose(U'>') == TokenKind::MultiLineBytesClose);
        REQUIRE(TokenKind::fromMultiLineOpen(U'>') == TokenKind::EndOfData);
        REQUIRE(TokenKind::fromMultiLineClose(U'<') == TokenKind::EndOfData);
        REQUIRE(TokenKind::fromMultiLineOpen(U'\0') == TokenKind::EndOfData);
        REQUIRE(TokenKind::fromMultiLineClose(U'\0') == TokenKind::EndOfData);
    }

    void testStandardizedNames() {
        struct NamedKind {
            TokenKind kind;
            el::String name;
        };
        for (
            const auto &[kind, name] :
            {NamedKind{TokenKind::EndOfData, "EndOfData"_el},
                NamedKind{TokenKind::LineBreak, "LineBreak"_el},
                NamedKind{TokenKind::Spacing, "Spacing"_el},
                NamedKind{TokenKind::Indentation, "Indentation"_el},
                NamedKind{TokenKind::Comment, "Comment"_el},
                NamedKind{TokenKind::RegularName, "RegularName"_el},
                NamedKind{TokenKind::TextName, "TextName"_el},
                NamedKind{TokenKind::MetaName, "MetaName"_el},
                NamedKind{TokenKind::NameValueSeparator, "NameValueSeparator"_el},
                NamedKind{TokenKind::ValueListSeparator, "ValueListSeparator"_el},
                NamedKind{TokenKind::MultiLineValueListSeparator, "MultiLineValueListSeparator"_el},
                NamedKind{TokenKind::NamePathSeparator, "NamePathSeparator"_el},
                NamedKind{TokenKind::Integer, "Integer"_el},
                NamedKind{TokenKind::Boolean, "Boolean"_el},
                NamedKind{TokenKind::Float, "Float"_el},
                NamedKind{TokenKind::Text, "Text"_el},
                NamedKind{TokenKind::MultiLineTextOpen, "MultiLineTextOpen"_el},
                NamedKind{TokenKind::MultiLineTextClose, "MultiLineTextClose"_el},
                NamedKind{TokenKind::MultiLineText, "MultiLineText"_el},
                NamedKind{TokenKind::Code, "Code"_el},
                NamedKind{TokenKind::MultiLineCodeOpen, "MultiLineCodeOpen"_el},
                NamedKind{TokenKind::MultiLineCodeLanguage, "MultiLineCodeLanguage"_el},
                NamedKind{TokenKind::MultiLineCodeClose, "MultiLineCodeClose"_el},
                NamedKind{TokenKind::MultiLineCode, "MultiLineCode"_el},
                NamedKind{TokenKind::RegEx, "RegEx"_el},
                NamedKind{TokenKind::MultiLineRegexOpen, "MultiLineRegexOpen"_el},
                NamedKind{TokenKind::MultiLineRegexClose, "MultiLineRegexClose"_el},
                NamedKind{TokenKind::MultiLineRegex, "MultiLineRegex"_el},
                NamedKind{TokenKind::Bytes, "Bytes"_el},
                NamedKind{TokenKind::MultiLineBytesOpen, "MultiLineBytesOpen"_el},
                NamedKind{TokenKind::MultiLineBytesFormat, "MultiLineBytesFormat"_el},
                NamedKind{TokenKind::MultiLineBytesClose, "MultiLineBytesClose"_el},
                NamedKind{TokenKind::MultiLineBytes, "MultiLineBytes"_el},
                NamedKind{TokenKind::Date, "Date"_el},
                NamedKind{TokenKind::Time, "Time"_el},
                NamedKind{TokenKind::DateTime, "DateTime"_el},
                NamedKind{TokenKind::TimeDelta, "TimeDelta"_el},
                NamedKind{TokenKind::SectionMapOpen, "SectionMapOpen"_el},
                NamedKind{TokenKind::SectionMapClose, "SectionMapClose"_el},
                NamedKind{TokenKind::SectionListOpen, "SectionListOpen"_el},
                NamedKind{TokenKind::SectionListClose, "SectionListClose"_el},
                NamedKind{TokenKind::Error, "Error"_el},
                NamedKind{TokenKind::SectionName, "SectionName"_el},
                NamedKind{TokenKind::SectionTextName, "SectionTextName"_el}}) {
            REQUIRE_EQUAL(kind.toText(), name);
        }
        REQUIRE_EQUAL(TokenKind{}.toText(), "Error"_el);
    }

    void testConstructionAndComparison() {
        constexpr auto error = TokenKind{};
        constexpr auto end = TokenKind{TokenKind::EndOfData};
        static_assert(error == TokenKind::Error);
        constexpr auto text = TokenKind{TokenKind::Text};
        static_assert(end == TokenKind::EndOfData);
        static_assert(text.raw() == TokenKind::Text);
        static_assert(text == TokenKind{TokenKind::Text});
        static_assert(text != end);
        static_assert(text != TokenKind::EndOfData);
        static_assert(TokenKind::Text == text);
        static_assert(TokenKind::EndOfData != text);
        REQUIRE(end.toValueType().isUndefined());
        REQUIRE_FALSE(end.isValue());
        REQUIRE_FALSE(end.isPayload());
    }

    void testCompleteLiteralsAndMultilineParts() {
        struct Literal {
            TokenKind kind;
            ValueType type;
        };
        for (
            const auto &[kind, type] :
            {Literal{TokenKind::Integer, ValueType::Integer},
                Literal{TokenKind::Boolean, ValueType::Boolean},
                Literal{TokenKind::Float, ValueType::Float},
                Literal{TokenKind::Text, ValueType::Text},
                Literal{TokenKind::Code, ValueType::Text},
                Literal{TokenKind::RegEx, ValueType::RegEx},
                Literal{TokenKind::Bytes, ValueType::Bytes},
                Literal{TokenKind::Date, ValueType::Date},
                Literal{TokenKind::Time, ValueType::Time},
                Literal{TokenKind::DateTime, ValueType::DateTime},
                Literal{TokenKind::TimeDelta, ValueType::TimeDelta}}) {
            REQUIRE(kind.isValue());
            REQUIRE_FALSE(kind.isPayload());
            REQUIRE(kind.toValueType() == type);
        }

        for (
            const auto &[kind, type] :
            {Literal{TokenKind::MultiLineTextOpen, ValueType::Text},
                Literal{TokenKind::MultiLineCodeOpen, ValueType::Text},
                Literal{TokenKind::MultiLineRegexOpen, ValueType::RegEx},
                Literal{TokenKind::MultiLineBytesOpen, ValueType::Bytes}}) {
            REQUIRE_FALSE(kind.isValue());
            REQUIRE_FALSE(kind.isPayload());
            REQUIRE(kind.toValueType() == type);
        }

        for (
            const TokenKind kind :
            {TokenKind::MultiLineText,
                TokenKind::MultiLineCode,
                TokenKind::MultiLineRegex,
                TokenKind::MultiLineBytes}) {
            REQUIRE_FALSE(kind.isValue());
            REQUIRE(kind.isPayload());
            REQUIRE(kind.toValueType().isUndefined());
        }
    }

    void testStructuralTokensHaveNoValueType() {
        for (
            const TokenKind kind :
            {TokenKind::EndOfData,
                TokenKind::LineBreak,
                TokenKind::Spacing,
                TokenKind::Indentation,
                TokenKind::Comment,
                TokenKind::RegularName,
                TokenKind::TextName,
                TokenKind::MetaName,
                TokenKind::NameValueSeparator,
                TokenKind::ValueListSeparator,
                TokenKind::MultiLineValueListSeparator,
                TokenKind::NamePathSeparator,
                TokenKind::MultiLineTextClose,
                TokenKind::MultiLineCodeLanguage,
                TokenKind::MultiLineCodeClose,
                TokenKind::MultiLineRegexClose,
                TokenKind::MultiLineBytesFormat,
                TokenKind::MultiLineBytesClose,
                TokenKind::SectionMapOpen,
                TokenKind::SectionMapClose,
                TokenKind::SectionListOpen,
                TokenKind::SectionListClose,
                TokenKind::Error,
                TokenKind::SectionName,
                TokenKind::SectionTextName}) {
            REQUIRE_FALSE(kind.isValue());
            REQUIRE_FALSE(kind.isPayload());
            REQUIRE(kind.toValueType().isUndefined());
        }
    }
};
