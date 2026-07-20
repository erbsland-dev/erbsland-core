// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/time/all.hpp>
#include <erbsland/time/StdFormatForTime.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <limits>

using namespace el::time;

using namespace el::text::literals;

TESTED_TARGETS(CalendarDelta TimeDelta TimeDeltaFormat DateTime)
class CalendarDeltaTest final : public el::UnitTest {
public:
    void testPartsAndArithmetic() {

        auto delta = CalendarDelta{};
        REQUIRE(delta.isZero());
        delta.setNanoseconds(Nanoseconds{1})
            .setMicroseconds(Microseconds{-2})
            .setMilliseconds(Milliseconds{3})
            .setSeconds(Seconds{-4})
            .setMinutes(Minutes{5})
            .setHours(Hours{-6})
            .setDays(Days{7})
            .setWeeks(Weeks{-8})
            .setMonths(Months{9})
            .setYears(Years{-10});
        REQUIRE_FALSE(delta.isZero());
        REQUIRE_EQUAL(delta.nanoseconds(), Nanoseconds{1});
        REQUIRE_EQUAL(delta.microseconds(), Microseconds{-2});
        REQUIRE_EQUAL(delta.months(), Months{9});
        REQUIRE_EQUAL(delta.years(), Years{-10});

        const auto other = CalendarDelta{CalendarDelta::Parts{.seconds = Seconds{4}, .months = Months{-2}}};
        const auto sum = delta + other;
        REQUIRE_EQUAL(sum.seconds(), Seconds{0});
        REQUIRE_EQUAL(sum.months(), Months{7});
        REQUIRE_EQUAL((sum - other), delta);
        REQUIRE_EQUAL((-other).seconds(), Seconds{-4});
        REQUIRE_EQUAL((-other).months(), Months{2});
    }

    void testTimeDeltaConversion() {

        const auto fixed = CalendarDelta{CalendarDelta::Parts{
            .nanoseconds = Nanoseconds{5}, .seconds = Seconds{-1}, .minutes = Minutes{2}, .days = Days{1}}};
        REQUIRE(fixed.isValidTimeDelta());
        REQUIRE_EQUAL(
            fixed.toTimeDeltaOrThrow(), TimeDelta{Nanoseconds{(24 * 60 * 60 + 2 * 60 - 1) * 1'000'000'000LL + 5}});
        REQUIRE(fixed.toTimeDelta().has_value());

        const auto calendar = CalendarDelta{Months{1}};
        REQUIRE_FALSE(calendar.isValidTimeDelta());
        REQUIRE_FALSE(calendar.toTimeDelta().has_value());
        REQUIRE_THROWS(calendar.toTimeDeltaOrThrow());

        const auto overflow = CalendarDelta{CalendarDelta::Parts{
            .nanoseconds = Nanoseconds{std::numeric_limits<int64_t>::max()}, .seconds = Seconds{1}}};
        REQUIRE_FALSE(overflow.isValidTimeDelta());
        REQUIRE_THROWS(overflow.toTimeDeltaOrThrow());
    }

    void testTimeDeltaFormatting() {

        const auto value = TimeDelta{Nanoseconds{90'061'002'003'004LL}};
        REQUIRE_EQUAL(value.toString(), "1 d 1 h 1 min 1 s 2 ms 3 us 4 ns"_el);
        REQUIRE_EQUAL(
            value.toString(TimeDeltaFormat::longUnits()),
            "1 day 1 hour 1 minute 1 second 2 milliseconds 3 microseconds 4 nanoseconds"_el);
        REQUIRE_EQUAL(value.toString(TimeDeltaFormat::elcl()), "1d, 1h, 1m, 1s, 2ms, 3us, 4ns"_el);

        auto seconds = TimeDeltaFormat::shortUnits();
        seconds.setSmallestUnit(TimeDeltaUnit::Seconds).setShowFractions(true).setMaximumFractionDigits(3);
        REQUIRE_EQUAL(value.toString(seconds), "1 d 1 h 1 min 1.002 s"_el);
        REQUIRE_EQUAL(TimeDelta{Milliseconds{500}}.toString(seconds), "0.5 s"_el);
        REQUIRE_EQUAL(TimeDelta{Milliseconds{-500}}.toString(seconds), "-0.5 s"_el);
        REQUIRE_EQUAL(TimeDelta{Milliseconds{-1500}}.toString(seconds), "-1.5 s"_el);

        auto longSeconds = seconds;
        longSeconds.setUnitStyle(TimeDeltaFormat::UnitStyle::Long);
        REQUIRE_EQUAL(TimeDelta{Milliseconds{1500}}.toString(longSeconds), "1.5 seconds"_el);

        seconds.setShowFractions(false).setValueSeparator(":"_el).setUnitSeparator("|"_el);
        REQUIRE_EQUAL(value.toString(seconds), "1:d|1:h|1:min|1:s"_el);
        REQUIRE_EQUAL(TimeDelta{}.toString(), "0 s"_el);
    }

    void testCalendarFormatting() {

        const auto value = CalendarDelta{CalendarDelta::Parts{
            .minutes = Minutes{-30},
            .hours = Hours{2},
            .days = Days{6},
            .weeks = Weeks{1},
            .months = Months{-2},
            .years = Years{1}}};
        REQUIRE_EQUAL(value.toString(), "1 y -2 mo 1 w 6 d 1 h 30 min"_el);
        REQUIRE_EQUAL(value.toString(TimeDeltaFormat::elcl()), "1year, -2month, 1w, 6d, 1h, 30m"_el);
        REQUIRE_EQUAL(
            (CalendarDelta{CalendarDelta::Parts{.minutes = Minutes{-90}, .hours = Hours{1}}}.toString()), "-30 min"_el);
        REQUIRE_EQUAL(CalendarDelta{}.toString(), "0 s"_el);

        const auto beyondTimeDelta = CalendarDelta{CalendarDelta::Parts{
            .days = Days{std::numeric_limits<int64_t>::max()}, .weeks = Weeks{std::numeric_limits<int64_t>::max()}}};
        REQUIRE_FALSE(beyondTimeDelta.isValidTimeDelta());
        REQUIRE_EQUAL(beyondTimeDelta.toString(), "10540996613548315208 w"_el);
        REQUIRE_EQUAL(
            CalendarDelta{Weeks{std::numeric_limits<int64_t>::min()}}.toString(), "-9223372036854775808 w"_el);
    }

    void testDateTimeApplicationOrderAndBounds() {

        const auto start = DateTime{
            Date::fromYearMonthDay(2025, 1, 31),
            TimeWithZone{Time{Hour{23}, Minute{59}, Second{59}, Nanoseconds{999'999'999}}}};
        const auto ordered =
            start.added(CalendarDelta{CalendarDelta::Parts{.nanoseconds = Nanoseconds{1}, .months = Months{1}}});
        REQUIRE_EQUAL(ordered.date(), Date::fromYearMonthDay(2025, 3, 1));
        REQUIRE_EQUAL(ordered.time(), Time{});

        const auto clamped =
            DateTime{Date::fromYearMonthDay(2024, 1, 31), TimeWithZone{Time{Hour{12}, Minute{0}}}}.added(
                CalendarDelta{Months{1}});
        REQUIRE_EQUAL(clamped.date(), Date::fromYearMonthDay(2024, 2, 29));
        REQUIRE_EQUAL(clamped.hour(), Hour{12});

        REQUIRE_EQUAL(DateTime::last().added(CalendarDelta{Days{1}}), DateTime::last());
        REQUIRE(DateTime::last().wouldAddSaturate(CalendarDelta{Days{1}}));
        REQUIRE_THROWS(DateTime::last().addedOrThrow(CalendarDelta{Days{1}}));
    }

    void testNamedZoneRefresh() {

        const auto zurich = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        const auto winter = DateTime{Date::fromYearMonthDay(2026, 1, 1), Time{Hour{12}, Minute{0}}, zurich};
        const auto summer = winter.added(CalendarDelta{Months{6}});
        REQUIRE(summer.timeZone().isNamed());
        REQUIRE_EQUAL(summer.timeOffset(), Duration{Hours{2}});
    }
};
