// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "LexerValueTestHelper.hpp"

#include <erbsland/time/CalendarDelta.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Lexer)
TAGS(el::time::CalendarDelta)
class LexerAdvancedTimeDeltaTest final : public UNITTEST_SUBCLASS(LexerValueTestHelper) {
public:
    void testIntegerSyntax() {
        WITH_CONTEXT(verifyValidTimeDelta("0s"_el, el::time::CalendarDelta{}));
        WITH_CONTEXT(verifyValidTimeDelta("+1 s"_el, el::time::CalendarDelta{el::time::Seconds{1}}));
        WITH_CONTEXT(verifyValidTimeDelta("-529'000s"_el, el::time::CalendarDelta{el::time::Seconds{-529'000}}));
        WITH_CONTEXT(verifyValidTimeDelta(
            "-9223372036854775808ns"_el,
            el::time::CalendarDelta{el::time::Nanoseconds{std::numeric_limits<int64_t>::min()}}));
        WITH_CONTEXT(verifyValidTimeDelta(
            "9223372036854775807ns"_el,
            el::time::CalendarDelta{el::time::Nanoseconds{std::numeric_limits<int64_t>::max()}}));
    }

    void testSupportedUnits() {
        WITH_CONTEXT(verifyValidTimeDelta("123 nanoseconds"_el, el::time::CalendarDelta{el::time::Nanoseconds{123}}));
        WITH_CONTEXT(verifyValidTimeDelta("123 µs"_el, el::time::CalendarDelta{el::time::Microseconds{123}}));
        WITH_CONTEXT(verifyValidTimeDelta("123 ms"_el, el::time::CalendarDelta{el::time::Milliseconds{123}}));
        WITH_CONTEXT(verifyValidTimeDelta("123 seconds"_el, el::time::CalendarDelta{el::time::Seconds{123}}));
        WITH_CONTEXT(verifyValidTimeDelta("123 m"_el, el::time::CalendarDelta{el::time::Minutes{123}}));
        WITH_CONTEXT(verifyValidTimeDelta("123 HOURS"_el, el::time::CalendarDelta{el::time::Hours{123}}));
        WITH_CONTEXT(verifyValidTimeDelta("123 days"_el, el::time::CalendarDelta{el::time::Days{123}}));
        WITH_CONTEXT(verifyValidTimeDelta("123 W"_el, el::time::CalendarDelta{el::time::Weeks{123}}));
    }

    void testCalendarUnits() {
        WITH_CONTEXT(verifyValidTimeDelta("1 month"_el, el::time::CalendarDelta{el::time::Months{1}}));
        WITH_CONTEXT(verifyValidTimeDelta("2 months"_el, el::time::CalendarDelta{el::time::Months{2}}));
        WITH_CONTEXT(verifyValidTimeDelta("1 year"_el, el::time::CalendarDelta{el::time::Years{1}}));
        WITH_CONTEXT(verifyValidTimeDelta("2 YEARS"_el, el::time::CalendarDelta{el::time::Years{2}}));
    }

    void testRepresentationalLimits() {
        WITH_CONTEXT(verifyValidTimeDelta(
            "9223372036854775807s"_el,
            el::time::CalendarDelta{el::time::Seconds{std::numeric_limits<int64_t>::max()}}));
        WITH_CONTEXT(verifyValidTimeDelta(
            "-9223372036854775808w"_el, el::time::CalendarDelta{el::time::Weeks{std::numeric_limits<int64_t>::min()}}));
        WITH_CONTEXT(verifyErrorInValue("9223372036854775808s"_el, ConfErrorCategory::LimitExceeded));
    }
};
