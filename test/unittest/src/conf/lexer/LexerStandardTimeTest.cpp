// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(el::time::Time el::time::TimeWithZone)
class LexerStandardTimeTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testValidTimes() {
        struct TestCase {
            el::text::String text;
            int hour;
            int minute;
            int second;
            int nanosecond;
        };
        const auto cases = std::to_array<TestCase>({
            {"00:00"_el, 0, 0, 0, 0},
            {"T23:59"_el, 23, 59, 0, 0},
            {"t14:08:32"_el, 14, 8, 32, 0},
            {"14:08:32.1"_el, 14, 8, 32, 100'000'000},
            {"14:08:32.123"_el, 14, 8, 32, 123'000'000},
            {"14:08:32.123456"_el, 14, 8, 32, 123'456'000},
            {"14:08:32.123456789"_el, 14, 8, 32, 123'456'789},
        });
        for (const auto &testCase : cases) {
            WITH_CONTEXT(verifyValidTime(
                testCase.text, makeTime(testCase.hour, testCase.minute, testCase.second, testCase.nanosecond)));
        }
    }

    void testInvalidTimes() {
        const auto cases = std::to_array<el::text::String>(
            {"24:00"_el, "12:60"_el, "12:30:60"_el, "12:30:20.1234567890"_el, "T:30"_el});
        for (const auto &testCase : cases) {
            WITH_CONTEXT(verifyErrorInValue(testCase, ConfErrorCategory::Syntax));
        }
    }

    void testOffsets() {
        struct TestCase {
            el::text::String text;
            int offset;
        };
        const auto cases = std::to_array<TestCase>({
            {"12:30Z"_el, 0},
            {"12:30z"_el, 0},
            {"T12:30+02"_el, 7200},
            {"12:30-02:30"_el, -9000},
            {"12:30+23:59"_el, 86'340},
            {"12:30-23:59"_el, -86'340},
        });
        for (const auto &testCase : cases) {
            WITH_CONTEXT(verifyValidTime(testCase.text, makeTimeWithZone(12, 30, 0, 0, testCase.offset)));
        }
        for (const auto &testCase : std::to_array<el::text::String>({"12:30+24"_el, "12:30-24"_el, "12:30+00:60"_el})) {
            WITH_CONTEXT(verifyErrorInValue(testCase, ConfErrorCategory::Syntax));
        }
    }
};
