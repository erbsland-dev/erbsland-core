// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/time/all.hpp>
#include <erbsland/time/StdFormatForTime.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::time;

using namespace el::text::literals;

TESTED_TARGETS(TimeWithZone DateTime)
class TimeWithZoneTest final : public el::UnitTest {
public:
    void testConstructionAndRendering() {

        const auto empty = TimeWithZone{};
        REQUIRE_EQUAL(empty.time(), Time{});
        REQUIRE(empty.timeZone().isUtc());
        REQUIRE_EQUAL(empty.toString(), "00:00:00Z"_el);

        const auto time = Time{Hour{17}, Minute{37}, Second{14}, Nanoseconds{123'400'000}};
        const auto utc = TimeWithZone{time};
        const auto positive = TimeWithZone{time, TimeZone{Hours{14}}};
        const auto withSeconds = TimeWithZone{time, TimeZone{Hours{-5}, Minutes{-30}, Seconds{-45}}};
        REQUIRE_EQUAL(utc.toString(), "17:37:14.1234Z"_el);
        REQUIRE_EQUAL(positive.toString(), "17:37:14.1234+14:00"_el);
        REQUIRE_EQUAL(withSeconds.toString(), "17:37:14.1234-05:30:45"_el);
        REQUIRE_EQUAL(positive.hour(), Hour{17});
        REQUIRE_EQUAL(positive.minute(), Minute{37});
        REQUIRE_EQUAL(positive.second(), Second{14});
        REQUIRE_EQUAL(positive.nanosecondFraction(), Nanoseconds{123'400'000});
        REQUIRE(positive == TimeWithZone{time, TimeZone{Hours{14}}});
        REQUIRE_FALSE(positive == utc);

        const auto zurich = TimeWithZone{time, TimeZone::fromNameOrThrow("Europe/Zurich"_el)};
        REQUIRE_EQUAL(zurich.toString(), "17:37:14.1234[Europe/Zurich]"_el);
        const auto local = TimeWithZone{time, TimeZone::local()};
        REQUIRE_EQUAL(local.toString(), "17:37:14.1234"_el);
    }

    void testDateTimeConversionAndRollover() {

        const auto localDate = Date::fromYearMonthDay(2026, 1, 1);
        const auto localTime = TimeWithZone{Time{Hour{1}, Minute{30}}, TimeZone{Hours{14}}};
        const auto dateTime = DateTime{localDate, localTime};
        REQUIRE_EQUAL(dateTime.date(), localDate);
        REQUIRE_EQUAL(dateTime.time(), localTime.time());
        REQUIRE_EQUAL(dateTime.utcDate(), Date::fromYearMonthDay(2025, 12, 31));
        REQUIRE_EQUAL(dateTime.utcTime(), (Time{Hour{11}, Minute{30}}));
        REQUIRE_EQUAL(dateTime.timeOffset(), Duration{Hours{14}});
        REQUIRE_EQUAL(dateTime.toString(), "2026-01-01 01:30:00+14:00"_el);

        const auto negative = DateTime{
            Date::fromYearMonthDay(2025, 12, 31),
            TimeWithZone{Time{Hour{23}, Minute{0}}, TimeZone{Hours{-23}, Minutes{-59}}}};
        REQUIRE_EQUAL(negative.utcDate(), Date::fromYearMonthDay(2026, 1, 1));
        REQUIRE_EQUAL(negative.utcTime(), (Time{Hour{22}, Minute{59}}));
    }
};
