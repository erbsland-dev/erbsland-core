// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(el::time::DateTime)
class LexerStandardDateTimeTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testUtcDateTimes() {
        // Offsetless date-times are shortcuts for instants in the system-local zone.
        WITH_CONTEXT(verifyValidDateTime("2025-04-21 19:37"_el, makeDateTime(2025, 4, 21, 19, 37)));
        WITH_CONTEXT(verifyValidDateTime("2025-04-21t19:37:03"_el, makeDateTime(2025, 4, 21, 19, 37, 3)));
        WITH_CONTEXT(verifyValidDateTime("2025-04-21T19:37:03Z"_el, makeDateTime(2025, 4, 21, 19, 37, 3, 0, 0)));
        WITH_CONTEXT(verifyValidDateTime("2025-04-21T19:37:03z"_el, makeDateTime(2025, 4, 21, 19, 37, 3, 0, 0)));
        WITH_CONTEXT(
            verifyValidDateTime("2025-04-21T19:37:03.123456789"_el, makeDateTime(2025, 4, 21, 19, 37, 3, 123'456'789)));
        WITH_CONTEXT(
            verifyValidDateTime("2025-04-21T19:37:03.1"_el, makeDateTime(2025, 4, 21, 19, 37, 3, 100'000'000)));
        WITH_CONTEXT(
            verifyValidDateTime("2025-04-21T19:37:03.123456"_el, makeDateTime(2025, 4, 21, 19, 37, 3, 123'456'000)));
    }

    void testNumericOffsets() {
        WITH_CONTEXT(
            verifyValidDateTime("2025-04-21T19:37:03+02"_el, makeDateTime(2025, 4, 21, 19, 37, 3, 0, 2 * 60 * 60)));
        WITH_CONTEXT(verifyValidDateTime(
            "2025-04-21T19:37:03-02:30"_el, makeDateTime(2025, 4, 21, 19, 37, 3, 0, -(2 * 60 + 30) * 60)));
        WITH_CONTEXT(
            verifyValidDateTime("2025-04-21 19:37+00:59"_el, makeDateTime(2025, 4, 21, 19, 37, 0, 0, 59 * 60)));
        WITH_CONTEXT(
            verifyValidDateTime("0001-01-01T00:00-23:59"_el, makeDateTime(1, 1, 1, 0, 0, 0, 0, -(23 * 60 + 59) * 60)));
        WITH_CONTEXT(verifyValidDateTime(
            "9999-12-31T23:59+23:59"_el, makeDateTime(9999, 12, 31, 23, 59, 0, 0, (23 * 60 + 59) * 60)));
    }

    void testInvalidDateTimes() {
        const auto cases = std::to_array<el::text::String>({
            "0000-01-01T00:00"_el,
            "2025-02-29T00:00"_el,
            "2024-02-29T24:00"_el,
            "2024-02-29T12:60"_el,
            "2024-02-29T12:00:60"_el,
            "2024-02-29T12:00+24:00"_el,
            "2024-02-29T12:00-24:00"_el,
            "2024-02-29T12:00+01:60"_el,
            "2024-02-29T12:00:00.1234567890"_el,
        });
        for (const auto &testCase : cases) {
            WITH_CONTEXT(verifyErrorInValue(testCase, ConfErrorCategory::Syntax));
        }
    }
};
