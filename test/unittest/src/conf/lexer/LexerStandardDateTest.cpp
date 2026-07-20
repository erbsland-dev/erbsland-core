// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(el::time::Date)
class LexerStandardDateTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testValidDates() {
        struct TestCase {
            el::text::String text;
            int year;
            int month;
            int day;
        };
        const auto cases = std::to_array<TestCase>({
            {"0001-01-01"_el, 1, 1, 1},
            {"9999-12-31"_el, 9999, 12, 31},
            {"0004-02-29"_el, 4, 2, 29},
            {"2000-02-29"_el, 2000, 2, 29},
            {"2004-02-29"_el, 2004, 2, 29},
            {"2400-02-29"_el, 2400, 2, 29},
            {"2019-02-28"_el, 2019, 2, 28},
            {"2024-01-31"_el, 2024, 1, 31},
            {"2024-04-30"_el, 2024, 4, 30},
        });
        for (const auto &testCase : cases) {
            WITH_CONTEXT(verifyValidDate(testCase.text, makeDate(testCase.year, testCase.month, testCase.day)));
        }
    }

    void testInvalidDates() {
        const auto cases = std::to_array<el::text::String>({
            "0000-01-01"_el,
            "0001-02-29"_el,
            "1900-02-29"_el,
            "2100-02-29"_el,
            "2020-02-30"_el,
            "2020-04-31"_el,
            "2020-00-10"_el,
            "2020-13-01"_el,
            "2020-01-00"_el,
            "2020-01-32"_el,
        });
        for (const auto &testCase : cases) {
            WITH_CONTEXT(verifyErrorInValue(testCase, ConfErrorCategory::Syntax));
        }
    }
};
