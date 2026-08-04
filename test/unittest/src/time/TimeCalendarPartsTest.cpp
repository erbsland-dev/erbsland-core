// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/time/DateTime.hpp>
#include <erbsland/time/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>
#include <cstdint>
#include <limits>

using namespace el::time;

TESTED_TARGETS(Year Month Day DayOfYear DayOfWeek Hour Minute Second TimePartWithAmount Date Time DateTime)
class TimeCalendarPartsTest final : public el::UnitTest {
public:
    void testAmountLinkedParts() {
        static_assert(std::same_as<Year::Amount, Years>);
        static_assert(std::same_as<Month::Amount, Months>);
        static_assert(std::same_as<Day::Amount, Days>);
        static_assert(std::same_as<DayOfYear::Amount, Days>);
        static_assert(std::same_as<DayOfWeek::Amount, Days>);
        static_assert(std::same_as<Hour::Amount, Hours>);
        static_assert(std::same_as<Minute::Amount, Minutes>);
        static_assert(std::same_as<Second::Amount, Seconds>);

        REQUIRE_EQUAL(Year::fromAmount(Years{2024}), Year{2024});
        REQUIRE_EQUAL(Month::fromAmount(Months{0}), Month::january());
        REQUIRE_EQUAL(Day::fromAmount(Days{0}), Day{1});
        REQUIRE_EQUAL(DayOfYear::fromAmount(Days{365}), DayOfYear{366});
        REQUIRE_EQUAL(DayOfWeek::fromAmount(Days{0}), DayOfWeek::monday());
        REQUIRE_EQUAL(Hour::fromAmount(Hours{12}), Hour{12});
        REQUIRE_EQUAL(Minute::fromAmount(Minutes{34}), Minute{34});
        REQUIRE_EQUAL(Second::fromAmount(Seconds{56}), Second{56});

        REQUIRE_EQUAL(Month::january().toAmount(), Months{0});
        REQUIRE_EQUAL(Day{1}.toAmount(), Days{0});
        REQUIRE_EQUAL(DayOfYear{366}.toAmount(), Days{365});
        REQUIRE_EQUAL(DayOfWeek::sunday().toAmount(), Days{6});
        REQUIRE_EQUAL(Hour{23}.toAmount(), Hours{23});
        REQUIRE_EQUAL(Minute{59}.toAmount(), Minutes{59});
        REQUIRE_EQUAL(Second{59}.toAmount(), Seconds{59});

        REQUIRE_EQUAL(Month::fromAmount(Months{-1}), Month::january());
        REQUIRE_EQUAL(Month::fromAmount(Months{12}), Month::december());
        REQUIRE_EQUAL(Day::fromAmount(Days{-1}), Day{1});
        REQUIRE_EQUAL(Day::fromAmount(Days{31}), Day{31});
        REQUIRE_EQUAL(DayOfWeek::fromAmount(Days{-1}), DayOfWeek::monday());
        REQUIRE_EQUAL(DayOfWeek::fromAmount(Days{7}), DayOfWeek::sunday());
    }

    void testAmountLinkedPartBounds() {
        REQUIRE_EQUAL(Month::fromAmountOrThrow(Months{0}), Month::january());
        REQUIRE_EQUAL(Month::fromAmountOrThrow(Months{11}), Month::december());
        REQUIRE_THROWS(Month::fromAmountOrThrow(Months{-1}));
        REQUIRE_THROWS(Month::fromAmountOrThrow(Months{12}));

        REQUIRE_EQUAL(Day::fromAmountOrThrow(Days{0}), Day{1});
        REQUIRE_EQUAL(Day::fromAmountOrThrow(Days{30}), Day{31});
        REQUIRE_THROWS(Day::fromAmountOrThrow(Days{-1}));
        REQUIRE_THROWS(Day::fromAmountOrThrow(Days{31}));

        REQUIRE_EQUAL(DayOfYear::fromAmountOrThrow(Days{0}), DayOfYear{1});
        REQUIRE_EQUAL(DayOfYear::fromAmountOrThrow(Days{365}), DayOfYear{366});
        REQUIRE_THROWS(DayOfYear::fromAmountOrThrow(Days{-1}));
        REQUIRE_THROWS(DayOfYear::fromAmountOrThrow(Days{366}));

        REQUIRE_EQUAL(DayOfWeek::fromAmountOrThrow(Days{0}), DayOfWeek::monday());
        REQUIRE_EQUAL(DayOfWeek::fromAmountOrThrow(Days{6}), DayOfWeek::sunday());
        REQUIRE_THROWS(DayOfWeek::fromAmountOrThrow(Days{-1}));
        REQUIRE_THROWS(DayOfWeek::fromAmountOrThrow(Days{7}));

        REQUIRE_EQUAL(Hour::fromAmountOrThrow(Hours{0}), Hour{0});
        REQUIRE_EQUAL(Hour::fromAmountOrThrow(Hours{23}), Hour{23});
        REQUIRE_THROWS(Hour::fromAmountOrThrow(Hours{-1}));
        REQUIRE_THROWS(Hour::fromAmountOrThrow(Hours{24}));

        REQUIRE_EQUAL(Year::fromAmountOrThrow(Years{0}), Year{0});
        REQUIRE_EQUAL(Year::fromAmountOrThrow(Years{9999}), Year{9999});
        REQUIRE_THROWS(Year::fromAmountOrThrow(Years{-1}));
        REQUIRE_THROWS(Year::fromAmountOrThrow(Years{10000}));
    }

    void testAmountLinkedPartComparisonAndArithmetic() {
        REQUIRE_EQUAL(Day{1}, Days{0});
        REQUIRE_EQUAL(Days{0}, Day{1});
        REQUIRE_GREATER(Month{2}, Months{0});
        REQUIRE_LESS(Months{0}, Month{2});
        REQUIRE_GREATER_EQUAL(DayOfWeek::sunday(), Days{6});
        REQUIRE_LESS_EQUAL(Days{6}, DayOfWeek::sunday());
        REQUIRE_NOT_EQUAL(Hour{3}, Hours{4});
        REQUIRE_GREATER(Hours{4}, Hour{3});

        REQUIRE_EQUAL(Month::january() + Months{1}, Month::february());
        REQUIRE_EQUAL(Month::december() + Months{1}, Month::december());
        REQUIRE_EQUAL(Day{1} - Days{1}, Day{1});
        REQUIRE_EQUAL(Day{30}.added(Days{1}), Day{31});
        REQUIRE_EQUAL(DayOfYear{366}.added(Days{1}), DayOfYear{366});
        REQUIRE_EQUAL(DayOfWeek::monday().subtracted(Days{1}), DayOfWeek::monday());
        REQUIRE_EQUAL(Hour{23}.added(Hours{1}), Hour{23});

        auto minute = Minute{10};
        minute += Minutes{5};
        REQUIRE_EQUAL(minute, Minute{15});
        minute -= Minutes{20};
        REQUIRE_EQUAL(minute, Minute{0});
    }

    void testYearCalendarHelpers() {
        static_assert(std::same_as<decltype(Year{1} + 1), Year>);
        static_assert(std::same_as<decltype(Year{1}.added(1)), Year>);
        REQUIRE(Year{2000}.isLeapYear());
        REQUIRE(!Year{1900}.isLeapYear());
        REQUIRE_EQUAL(Year{2024}.dayCount(), Days{366});
        REQUIRE_EQUAL(Year{2023}.dayCount(), Days{365});
        REQUIRE_EQUAL(Year{1}.daysSinceEpoch(), Days{366});
        REQUIRE_EQUAL(Year{400}.daysSinceEpoch(), Days{146097});
        REQUIRE_EQUAL(Year{9999}.next(), Year{9999});
        REQUIRE_EQUAL(Year{0}.previous(), Year{0});
        REQUIRE_EQUAL(Year{9999} + 1, Year{9999});
        REQUIRE_EQUAL(Year{0} - 1, Year{0});

        const auto yearParts = Year::extractFromEpoch(Days{146097});
        REQUIRE_EQUAL(yearParts.year, Year{400});
        REQUIRE_EQUAL(yearParts.dayOfYear, Days{0});
        const auto lastYearParts = Year::extractFromEpoch(Days{std::numeric_limits<int64_t>::max()});
        REQUIRE_EQUAL(lastYearParts.year, Year{9999});
        REQUIRE_EQUAL(lastYearParts.dayOfYear, Days{365});
    }

    void testMonthCalendarHelpers() {
        REQUIRE_EQUAL(Month::february().minimumDayCount(), Days{28});
        REQUIRE_EQUAL(Month::february().maximumDayCount(), Days{29});
        REQUIRE_EQUAL(Month::april().dayCount(Year{2024}), Days{30});
        REQUIRE_EQUAL(Month::february().dayCount(Year{2024}), Days{29});
        REQUIRE_EQUAL(Month::february().dayCount(Year{2023}), Days{28});
        REQUIRE_EQUAL(Month::february().lastDay(Year{2024}), Day{29});
        REQUIRE_EQUAL(Month::march().firstDayOfYear(Year{2024}), DayOfYear{61});
        REQUIRE_EQUAL(Month::march().lastDayOfYear(Year{2024}), DayOfYear{91});

        const auto nextMonthParts = Month::december().next(Year{2023});
        REQUIRE_EQUAL(nextMonthParts.year, Year{2024});
        REQUIRE_EQUAL(nextMonthParts.month, Month::january());
        const auto lastMonthParts = Month::december().next(Year{9999});
        REQUIRE_EQUAL(lastMonthParts.year, Year{9999});
        REQUIRE_EQUAL(lastMonthParts.month, Month::december());

        const auto monthDayParts = Month::extractMonthAndDay(Year{2024}, DayOfYear{60});
        REQUIRE_EQUAL(monthDayParts.month, Month::february());
        REQUIRE_EQUAL(monthDayParts.day, Day{29});
    }

    void testDayCalendarHelpers() {
        REQUIRE_EQUAL(Day::lastMinimum(), Day{28});
        REQUIRE_EQUAL(Day::lastMaximum(), Day{31});
        REQUIRE_EQUAL(DayOfYear::lastMinimum(), DayOfYear{365});
        REQUIRE_EQUAL(DayOfYear::lastMaximum(), DayOfYear{366});
        REQUIRE(DayOfYear{366}.isLast(Year{2024}));
        REQUIRE(DayOfYear{365}.isLast(Year{2023}));
        REQUIRE_EQUAL(DayOfYear{60}.toAmount(), Days{59});

        REQUIRE(Day{30}.exists(Year{2024}, Month::april()));
        REQUIRE(!Day{31}.exists(Year{2024}, Month::april()));
        REQUIRE(Day{31}.hasNext(Year{2024}, Month::december()));
        REQUIRE(!Day{31}.hasNext(Year{9999}, Month::december()));
        REQUIRE(!Day{1}.hasPrevious(Year{0}, Month::january()));
    }

    void testOverflowHardening() {
        REQUIRE_EQUAL(Date::first().added(Days{std::numeric_limits<int64_t>::min()}), Date::first());
        REQUIRE_EQUAL(Date::last().added(Days{std::numeric_limits<int64_t>::max()}), Date::last());
        REQUIRE_EQUAL(Date::first().added(Years{std::numeric_limits<int64_t>::min()}), Date::first());
        REQUIRE_EQUAL(Date::last().added(Years{std::numeric_limits<int64_t>::max()}), Date::last());

        auto time = Time{};
        const auto days = time.addWithWrap(TimeDelta{Nanoseconds{std::numeric_limits<int64_t>::max()}});
        REQUIRE(days.isPositive());
        const auto nanosecondsSinceMidnight = time.toNanosecondsSinceMidnight();
        const auto dayNanoseconds = Days{1}.converted<Nanoseconds>();
        REQUIRE_GREATER_EQUAL(nanosecondsSinceMidnight, Nanoseconds{0});
        REQUIRE_LESS(nanosecondsSinceMidnight, dayNanoseconds);

        REQUIRE_EQUAL(
            DateTime::first().added(Duration{Seconds{std::numeric_limits<int64_t>::min()}}), DateTime::first());
        REQUIRE_EQUAL(DateTime::last().added(Duration{Seconds{std::numeric_limits<int64_t>::max()}}), DateTime::last());
    }
};
