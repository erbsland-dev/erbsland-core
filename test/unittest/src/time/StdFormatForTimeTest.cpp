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

        REQUIRE_EQUAL(std::format("{}", date), std::string{"2026-07-23"});
        REQUIRE_EQUAL(std::format("{}", time), std::string{"14:05:09"});
        REQUIRE_EQUAL(std::format("{}", DateTime{date, time}), std::string{"2026-07-23 14:05:09Z"});
    }

    void testDeltaValues() {
        REQUIRE_EQUAL(std::format("{}", TimeDelta::minutes(90)), std::string{"1 h 30 min"});
        REQUIRE_EQUAL(std::format("{}", CalendarDelta{Months{2}}), std::string{"2 mo"});
    }
};
