// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(ValueList)
class LexerStandardValueListTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    enum class ListStyle : uint8_t {
        Compact,
        SpaceAfterComma,
        SpaceBeforeAndAfterComma,
        MultiLine,
    };

    using LexerValueTestHelper::to_string;
    static auto to_string(const ListStyle listStyle) -> std::string {
        switch (listStyle) {
        case ListStyle::Compact:
            return "Compact";
        case ListStyle::SpaceAfterComma:
            return "SpaceAfterComma";
        case ListStyle::SpaceBeforeAndAfterComma:
            return "SpaceBeforeAndAfterComma";
        case ListStyle::MultiLine:
            return "MultiLine";
        }
        return {};
    }

    static constexpr auto cListStyles = std::array<ListStyle, 4>{
        ListStyle::Compact,
        ListStyle::SpaceAfterComma,
        ListStyle::SpaceBeforeAndAfterComma,
        ListStyle::MultiLine,
    };

    struct TestValue {
        el::text::String valueText;
        TokenType expectedTokenType;
        bool isList = false;
    };

    using TestValueList = std::vector<TestValue>;

    auto createValueText(const TestValueList &testValueList, ListStyle listStyle, PrefixFormat prefixFormat)
        -> el::text::String {
        el::text::StringEditor result;
        for (const auto &testValue : testValueList) {
            if (!result.isEmpty()) {
                switch (listStyle) {
                case ListStyle::Compact:
                    result.append(","_el);
                    break;
                case ListStyle::SpaceAfterComma:
                    result.append(", "_el);
                    break;
                case ListStyle::SpaceBeforeAndAfterComma:
                    result.append(" , "_el);
                    break;
                case ListStyle::MultiLine:
                    break;
                default:
                    throw std::logic_error("List style not implemented");
                }
            }
            if (listStyle == ListStyle::MultiLine) {
                if (!result.isEmpty()) {
                    result.append("\n"_el);
                    result.append(indentForPrefix(prefixFormat));
                }
                result.append("* "_el);
            }
            result.append(testValue.valueText);
        }
        return result;
    }

    void verifyListTokens(const TestValueList &testValueList, ListStyle listStyle, PrefixFormat prefixFormat) {
        for (std::size_t i = 0; i < testValueList.size(); ++i) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    const auto &[valueText, expectedTokenType, isList] = testValueList[i];
                    if (listStyle == ListStyle::MultiLine) {
                        WITH_CONTEXT(requireNextToken(TokenType::MultiLineValueListSeparator, "*"_el));
                        if (isList) {
                            // This is a list of lists - and for the test we use three values of the same type.
                            WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
                            WITH_CONTEXT(requireNextToken(expectedTokenType));
                            WITH_CONTEXT(requireNextToken(TokenType::ValueListSeparator, ","_el));
                            WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
                            WITH_CONTEXT(requireNextToken(expectedTokenType));
                            WITH_CONTEXT(requireNextToken(TokenType::ValueListSeparator, ","_el));
                            WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
                            WITH_CONTEXT(requireNextToken(expectedTokenType));
                        } else {
                            WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
                            WITH_CONTEXT(requireNextToken(expectedTokenType, valueText));
                        }
                        if (i < testValueList.size() - 1) {
                            WITH_CONTEXT(requireNextToken(TokenType::LineBreak, "\n"_el));
                            WITH_CONTEXT(requireNextToken(TokenType::Indentation, indentForPrefix(prefixFormat)));
                        }
                    } else {
                        WITH_CONTEXT(requireNextToken(expectedTokenType, valueText));
                        if (i < testValueList.size() - 1) {
                            switch (listStyle) {
                            case ListStyle::Compact:
                                WITH_CONTEXT(requireNextToken(TokenType::ValueListSeparator, ","_el));
                                break;
                            case ListStyle::SpaceAfterComma:
                                WITH_CONTEXT(requireNextToken(TokenType::ValueListSeparator, ","_el));
                                WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
                                break;
                            case ListStyle::SpaceBeforeAndAfterComma:
                                WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
                                WITH_CONTEXT(requireNextToken(TokenType::ValueListSeparator, ","_el));
                                WITH_CONTEXT(requireNextToken(TokenType::Spacing, " "_el));
                                break;
                            default:
                                throw std::logic_error("List style not implemented");
                            }
                        }
                    }
                },
                [&]() { return std::format("i == {}", i); });
        }
    }

    void verifyValidValueList(const TestValueList &testValueList) {
        for (auto listStyle : cListStyles) {
            for (auto prefixFormat : cPrefixFormats) {
                if (listStyle == ListStyle::MultiLine && prefixFormat == PrefixFormat::SameLine) {
                    continue;
                }
                for (auto suffixFormat : cSuffixPatterns) {
                    runWithContext(
                        SOURCE_LOCATION(),
                        [&]() {
                            auto valueText = createValueText(testValueList, listStyle, prefixFormat);
                            setupTokenGeneratorForValueTest(valueText, prefixFormat, suffixFormat);
                            WITH_CONTEXT(verifyPrefix(prefixFormat));
                            WITH_CONTEXT(verifyListTokens(testValueList, listStyle, prefixFormat));
                            WITH_CONTEXT(verifySuffix(suffixFormat));
                        },
                        [&]() {
                            return std::format(
                                "Failed at: suffix={} prefix={}, listStyle={}",
                                to_string(suffixFormat),
                                to_string(prefixFormat),
                                to_string(listStyle));
                        });
                }
            }
        }
    }

    void verifyListOfLists(const TestValueList &testValueList) {
        constexpr auto listStyle = ListStyle::MultiLine;
        constexpr auto prefixFormat = PrefixFormat::NextLinePattern1;
        for (auto suffixFormat : cSuffixPatterns) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() {
                    auto valueText = createValueText(testValueList, listStyle, prefixFormat);
                    setupTokenGeneratorForValueTest(valueText, prefixFormat, suffixFormat);
                    WITH_CONTEXT(verifyPrefix(prefixFormat));
                    WITH_CONTEXT(verifyListTokens(testValueList, listStyle, prefixFormat));
                    WITH_CONTEXT(verifySuffix(suffixFormat));
                },
                [&]() {
                    return std::format(
                        "Failed at: suffix={} prefix={}, listStyle={}",
                        to_string(suffixFormat),
                        to_string(prefixFormat),
                        to_string(listStyle));
                });
        }
    }

    void testIntegerList() {
        const auto testValueList = TestValueList{
            {.valueText = "1"_el, .expectedTokenType = TokenType::Integer},
            {.valueText = "2"_el, .expectedTokenType = TokenType::Integer},
            {.valueText = "3"_el, .expectedTokenType = TokenType::Integer}};
        verifyValidValueList(testValueList);
    }

    void testTextList() {
        const auto testValueList = TestValueList{
            {.valueText = "\"one\""_el, .expectedTokenType = TokenType::Text},
            {.valueText = "\"two\""_el, .expectedTokenType = TokenType::Text},
            {.valueText = "\"three\""_el, .expectedTokenType = TokenType::Text}};
        verifyValidValueList(testValueList);
    }

    void testBooleanList() {
        const auto testValueList = TestValueList{
            {.valueText = "true"_el, .expectedTokenType = TokenType::Boolean},
            {.valueText = "off"_el, .expectedTokenType = TokenType::Boolean},
            {.valueText = "enabled"_el, .expectedTokenType = TokenType::Boolean}};
        verifyValidValueList(testValueList);
    }

    void testMixedList() {
        const auto testValueList = TestValueList{
            {.valueText = "192.21"_el, .expectedTokenType = TokenType::Float},
            {.valueText = "false"_el, .expectedTokenType = TokenType::Boolean},
            {.valueText = "\"text\""_el, .expectedTokenType = TokenType::Text}};
        verifyValidValueList(testValueList);
    }

    void testListOfLists() {
        const auto testValueList = TestValueList{
            {.valueText = "1, 2, 3"_el, .expectedTokenType = TokenType::Integer, .isList = true},
            {.valueText = "\"one\", \"two\", \"three\""_el, .expectedTokenType = TokenType::Text, .isList = true},
            {.valueText = "true, false, enabled"_el, .expectedTokenType = TokenType::Boolean, .isList = true},
            {.valueText = "192.21, 2.21, 0.0"_el, .expectedTokenType = TokenType::Float, .isList = true}};
        verifyListOfLists(testValueList);
    }
};
