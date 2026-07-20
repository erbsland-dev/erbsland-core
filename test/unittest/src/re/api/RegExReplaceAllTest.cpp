// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RegExBase.hpp"

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/re/RegExError.hpp>
#include <erbsland/re/StdFormatForRegEx.hpp>

#include <stdexcept>

using namespace el::re;

TESTED_TARGETS(RegEx replaceAll)
TAGS(Api)
class RegExReplaceAllTest final : public UNITTEST_SUBCLASS(RegExBase) {
public:
    void testReplaceAllExpression_BasicAndEdgeCases() {
        WITH_CONTEXT(requireCompile("a"_el));

        const auto testCases = ReplaceTestCases{
            // Basic replacement and preservation of non-matching parts.
            {"aXa"_el, "b"_el, "bXb"_el},
            // No matches.
            {"xyz"_el, "b"_el, "xyz"_el},
            // Replacement expression can be empty.
            {"aXa"_el, ""_el, "X"_el},
            // Empty input with non-empty replacement must still return empty (no match).
            {""_el, "b"_el, ""_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));

        // Special-case: both empty must return empty without requiring engine work.
        REQUIRE_EQUAL(regex->replaceAll(String{}, String{}), StringEditor{});
    }

    void testReplaceAllExpression_CaptureGroupsByIndexAndEmptyExpression() {
        WITH_CONTEXT(requireCompile("(a)(b(c))"_el));

        const auto testCases = ReplaceTestCases{
            // By-index.
            {"abc"_el, "{1}-{2}-{3}"_el, "a-bc-c"_el},
            // Full match.
            {"...abc..."_el, "[{0}]"_el, "...[abc]..."_el},
            // Empty replacement expression `{}` is an alias for `{0}`.
            {"...abc..."_el, "[{}]"_el, "...[abc]..."_el},
            // Adjacent expressions.
            {"abc"_el, "{3}{2}{1}"_el, "cbca"_el},
        };

        WITH_CONTEXT(requireReplaceAll(testCases));
    }

    void testReplaceAllExpression_NamedGroupsAndEscapedBraces() {
        WITH_CONTEXT(requireCompile("(?<first>a)(?<second>b)"_el));

        const auto testCases = ReplaceTestCases{
            // Named groups (case-folded).
            {"ab"_el, "{first}-{second}-{FIRST}-{SECOND}"_el, "a-b-a-b"_el},
            // Escaped braces in static text.
            {"ab"_el, "{{{first}}}-{{{second}}}"_el, "{a}-{b}"_el},
            {"ab"_el, "{{}}"_el, "{}"_el},
        };
        WITH_CONTEXT(requireReplaceAll(testCases));
    }

    void testReplaceAllExpression_Errors() {
        WITH_CONTEXT(requireCompile("(?<first>a)(b)"_el));

        // Parser errors (Format).
        {
            try {
                (void)regex->replaceAll("ab"_el, "}"_el);
                REQUIRE(false);
            } catch (const RegExError &e) {
                REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
                REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
                REQUIRE_EQUAL(e.description(), "Unexpected '}' in replacement expression"_el);
            }
        }
        {
            try {
                (void)regex->replaceAll("ab"_el, "{"_el);
                REQUIRE(false);
            } catch (const RegExError &e) {
                REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
                REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
                REQUIRE_EQUAL(e.description(), "Unterminated replacement expression"_el);
            }
        }
        {
            try {
                (void)regex->replaceAll("ab"_el, "{1a}"_el);
                REQUIRE(false);
            } catch (const RegExError &e) {
                REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
                REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
                REQUIRE_EQUAL(
                    e.description(), "Invalid character in replacement expression. Expected a group index"_el);
            }
        }
        {
            try {
                (void)regex->replaceAll("ab"_el, "{unknown}"_el);
                REQUIRE(false);
            } catch (const RegExError &e) {
                REQUIRE_EQUAL(e.category(), ErrorCategory::Format);
                REQUIRE_EQUAL(e.title(), "Failed to parse replacement expression"_el);
                REQUIRE_EQUAL(e.description(), "Group name not found in this pattern"_el);
            }
        }

        // Runtime error: group index out of range is reported by the match.
        REQUIRE_THROWS_AS(el::err::ParameterError, (void)regex->replaceAll("ab"_el, "{3}"_el));
    }

    void testReplaceAllFn_BasicAndEdgeCases() {
        WITH_CONTEXT(requireCompile("[a-z]+"_el));

        std::size_t calls = 0;
        const auto replaceFn = [&calls](const MatchPtr &match) -> StringEditor {
            calls += 1;
            auto result = StringEditor{"["_el};
            result.append(match->content());
            result.append("]"_el);
            return result;
        };
        REQUIRE_EQUAL(regex->replaceAll("ab cd"_el, replaceFn), StringEditor{"[ab] [cd]"_el});
        REQUIRE_EQUAL(calls, static_cast<std::size_t>(2));

        // No matches -> callback must not be called, input remains unchanged.
        calls = 0;
        REQUIRE_EQUAL(regex->replaceAll("123"_el, replaceFn), StringEditor{"123"_el});
        REQUIRE_EQUAL(calls, static_cast<std::size_t>(0));

        // Callback can return empty string.
        calls = 0;
        REQUIRE_EQUAL(
            regex->replaceAll(
                "ab cd"_el,
                [&calls](const MatchPtr &) -> StringEditor {
                    calls += 1;
                    return StringEditor{};
                }),
            StringEditor{" "_el});
        REQUIRE_EQUAL(calls, static_cast<std::size_t>(2));
    }

    void testReplaceAllFn_CallbackThrows() {
        WITH_CONTEXT(requireCompile("[a-z]+"_el));

        {
            std::size_t calls = 0;
            try {
                (void)regex->replaceAll("ab cd"_el, [&calls](const MatchPtr &) -> StringEditor {
                    calls += 1;
                    throw std::runtime_error{"replaceFn failed"};
                });
                REQUIRE(false);
            } catch (const std::runtime_error &e) {
                REQUIRE_EQUAL(std::string{e.what()}, std::string{"replaceFn failed"});
                REQUIRE_EQUAL(calls, static_cast<std::size_t>(1));
            }
        }

        {
            std::size_t calls = 0;
            try {
                (void)regex->replaceAll("ab cd"_el, [&calls](const MatchPtr &) -> StringEditor {
                    calls += 1;
                    if (calls == 2) {
                        throw std::runtime_error{"replaceFn failed (second match)"};
                    }
                    return StringEditor{"X"_el};
                });
                REQUIRE(false);
            } catch (const std::runtime_error &e) {
                REQUIRE_EQUAL(std::string{e.what()}, std::string{"replaceFn failed (second match)"});
                REQUIRE_EQUAL(calls, static_cast<std::size_t>(2));
            }
        }
    }
};
