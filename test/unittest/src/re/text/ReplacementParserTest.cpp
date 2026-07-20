// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../engine/MockStringMatch.hpp"
#include "../StringHelper.hpp"
#include "../TestHelper.hpp"

#include <erbsland/re/impl/Limits.hpp>
#include <erbsland/re/impl/text/ReplacementParser.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/re/StdFormatForRegEx.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>

using namespace el::re;
using impl::CaptureGroupNames;
using impl::ReplacementParser;

TESTED_TARGETS(ReplacementParser)
TAGS(Text Replacement)
class ReplacementParserTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    void testParse_EmptyText() {
        const CaptureGroupNames groupNames;
        auto parser = ReplacementParser{String{}, groupNames};
        REQUIRE_NOTHROW(parser.parse());
    }

    void testParse_StaticTextOnly() {
        const CaptureGroupNames groupNames;
        auto parser = ReplacementParser{String{"abc"_el}, groupNames};
        const auto replacement = parser.parse();

        StringEditor out;
        REQUIRE_NOTHROW(replacement.appendTo(out, nullptr));
        REQUIRE_EQUAL(out, StringEditor{"abc"_el});
    }

    void testParse_MalformedTextBecomesReplacement() {
        const auto malformed = re_test::string_helper::bytesToString({'a', 0xFFU, 'b'});
        const CaptureGroupNames groupNames;
        auto parser = ReplacementParser{String{malformed}, groupNames};
        const auto replacement = parser.parse();

        StringEditor out;
        replacement.appendTo(out, nullptr);
        REQUIRE_EQUAL(out, "a�b"_el);
    }

    void testParse_EscapedBraces() {
        const CaptureGroupNames groupNames;
        auto parser = ReplacementParser{String{"{{}}"_el}, groupNames};
        const auto replacement = parser.parse();

        StringEditor out;
        REQUIRE_NOTHROW(replacement.appendTo(out, nullptr));
        REQUIRE_EQUAL(out, StringEditor{"{}"_el});
    }

    void testParse_GroupReferencesByIndexByNameAndEmpty() {
        constexpr auto text = "abcd"_el;
        CaptureGroupList groups;
        groups.emplace_back(0, CaptureRange{0, 4}, String{});
        groups.emplace_back(1, CaptureRange{0, 2}, "a"_el);
        groups.emplace_back(2, CaptureRange{2, 4}, "b"_el);
        const auto match = std::make_shared<MockStringMatch>(std::move(groups), text);
        REQUIRE(match != nullptr);

        const CaptureGroupNames groupNames{
            StringEditor{"a"_el},
            StringEditor{"b"_el},
        };

        auto parser = ReplacementParser{String{"{a}-{b}-{0}-{2}-{1}-{}-{{-}}"_el}, groupNames};
        const auto replacement = parser.parse();

        StringEditor out;
        replacement.appendTo(out, match);
        REQUIRE_EQUAL(out, StringEditor{"ab-cd-abcd-cd-ab-abcd-{-}"_el});
    }

    void testParse_MixedAndAdjacentExpressions() {
        constexpr auto text = "xy"_el;
        CaptureGroupList groups;
        groups.emplace_back(0, CaptureRange{0, 2}, String{});
        groups.emplace_back(1, CaptureRange{0, 1}, "x"_el);
        groups.emplace_back(2, CaptureRange{1, 2}, "y"_el);
        const auto match = std::make_shared<MockStringMatch>(std::move(groups), text);
        REQUIRE(match != nullptr);

        const CaptureGroupNames groupNames{
            StringEditor{"x"_el},
            StringEditor{"y"_el},
        };

        auto parser = ReplacementParser{String{"{1}{2}{x}{y}{X}{Y}"_el}, groupNames};
        const auto replacement = parser.parse();

        StringEditor out;
        replacement.appendTo(out, match);
        REQUIRE_EQUAL(out, StringEditor{"xyxyxy"_el});
    }

    void testParse_Error_UnexpectedClosingBrace() {
        const CaptureGroupNames groupNames;
        auto parser = ReplacementParser{String{"}"_el}, groupNames};

        try {
            (void)parser.parse();
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
            REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
            REQUIRE_EQUAL(e.description(), "Unexpected '}' in replacement expression"_el);
            REQUIRE_EQUAL(e.position().toSizeT(), static_cast<std::size_t>(1));
        }
    }

    void testParse_Error_UnterminatedExpression() {
        const CaptureGroupNames groupNames;
        auto parser = ReplacementParser{String{"{"_el}, groupNames};

        try {
            (void)parser.parse();
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
            REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
            REQUIRE_EQUAL(e.description(), "Unterminated replacement expression"_el);
            REQUIRE_EQUAL(e.position().toSizeT(), static_cast<std::size_t>(1));
        }
    }

    void testParse_Error_InvalidCharacterInExpression() {
        const CaptureGroupNames groupNames;
        auto parser = ReplacementParser{String{"{!}"_el}, groupNames};

        try {
            (void)parser.parse();
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
            REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
            REQUIRE_EQUAL(e.description(), "Invalid character in replacement expression"_el);
            REQUIRE_EQUAL(e.position().toSizeT(), static_cast<std::size_t>(1));
        }
    }

    void testParse_Error_InvalidCharacterAfterGroupIndex() {
        const CaptureGroupNames groupNames;
        auto parser = ReplacementParser{String{"{1a}"_el}, groupNames};

        try {
            (void)parser.parse();
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
            REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
            REQUIRE_EQUAL(e.description(), "Invalid character in replacement expression. Expected a group index"_el);
            REQUIRE_EQUAL(e.position().toSizeT(), static_cast<std::size_t>(2));
        }
    }

    void testParse_Error_InvalidCharacterAfterGroupName() {
        const CaptureGroupNames groupNames;
        auto parser = ReplacementParser{String{"{ab-}"_el}, groupNames};

        try {
            (void)parser.parse();
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
            REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
            REQUIRE_EQUAL(e.description(), "Invalid character in replacement expression. Expected a group name"_el);
            REQUIRE_EQUAL(e.position().toSizeT(), static_cast<std::size_t>(3));
        }
    }

    void testParse_Error_UnknownGroupName() {
        const CaptureGroupNames groupNames{
            StringEditor{"a"_el},
        };
        auto parser = ReplacementParser{String{"{b}"_el}, groupNames};

        try {
            (void)parser.parse();
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
            REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
            REQUIRE_EQUAL(e.description(), "Group name not found in this pattern"_el);
            REQUIRE_EQUAL(e.position().toSizeT(), static_cast<std::size_t>(3));
        }
    }

    void testParse_Error_MaxReplacementTextLengthExceeded() {
        using impl::limits::maximumReplacementTextLength;

        const auto text = String::fromCharacter(U'a', maximumReplacementTextLength + el::unit::CpLength{2U});

        const CaptureGroupNames groupNames;
        auto parser = ReplacementParser{text, groupNames};

        try {
            (void)parser.parse();
            REQUIRE(false);
        } catch (const RegExError &e) {
            REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
            REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
            REQUIRE_EQUAL(
                e.description(),
                el::text::StringFormat{"The maximum replacement-expression length is {} characters."}.build(
                    maximumReplacementTextLength));
            REQUIRE_EQUAL(e.position().toSizeT(), maximumReplacementTextLength.toSizeT());
        }
    }
};
