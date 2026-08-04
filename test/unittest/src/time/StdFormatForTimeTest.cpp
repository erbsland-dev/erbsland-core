// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/time/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <string>

using namespace el::time;

TESTED_TARGETS(StdFormatForTime)
class StdFormatForTimeTest final : public el::UnitTest {
public:
    void testDateAndTimeValues() {
        const auto date = Date::fromYearMonthDay(2026, 7, 23);
        const auto time = Time{Hour{14}, Minute{5}, Second{9}};

        const auto formattedDate = std::format("{}", date);
        const auto formattedTime = std::format("{}", time);
        const auto formattedDateTime = std::format("{}", DateTime{date, time});
        REQUIRE_EQUAL(formattedDate, std::string{"2026-07-23"});
        REQUIRE_EQUAL(formattedTime, std::string{"14:05:09"});
        REQUIRE_EQUAL(formattedDateTime, std::string{"2026-07-23 14:05:09Z"});
    }

    void testDeltaValues() {
        const auto formattedTimeDelta = std::format("{}", TimeDelta::minutes(90));
        const auto formattedCalendarDelta = std::format("{}", CalendarDelta{Months{2}});
        REQUIRE_EQUAL(formattedTimeDelta, std::string{"1 h 30 min"});
        REQUIRE_EQUAL(formattedCalendarDelta, std::string{"2 mo"});
    }
};
