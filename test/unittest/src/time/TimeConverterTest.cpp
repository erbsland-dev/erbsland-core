// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/time/impl/DateTimeEpochs.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::time;

TESTED_TARGETS(DateTime DateTimeEpochs TimeEpoch)
class TimeConverterTest final : public el::UnitTest {
public:
    void testEpochs() {
        REQUIRE_EQUAL(DateTime::epoch(), (DateTime{Date::fromYearMonthDay(0, 1, 1), Time{}}));
        REQUIRE_EQUAL(DateTime::epoch(TimeEpoch::Posix), (DateTime{Date::fromYearMonthDay(1970, 1, 1), Time{}}));
        REQUIRE_EQUAL(DateTime::epoch(TimeEpoch::Windows), (DateTime{Date::fromYearMonthDay(1601, 1, 1), Time{}}));
        REQUIRE_EQUAL(DateTime::epoch(TimeEpoch::Rfc868), (DateTime{Date::fromYearMonthDay(1900, 1, 1), Time{}}));

        REQUIRE_EQUAL(el::time::impl::secondsSinceCoreEpoch(TimeEpoch::Core), Seconds{});
        REQUIRE_EQUAL(el::time::impl::secondsSinceCoreEpoch(TimeEpoch::Posix), Days{719528}.converted<Seconds>());
        REQUIRE_EQUAL(DateTime::fromTicks(Seconds{}, TimeEpoch::Windows).value(), DateTime::epoch(TimeEpoch::Windows));
        REQUIRE_EQUAL(DateTime::fromTicks(Seconds{}, TimeEpoch::Rfc868).value(), DateTime::epoch(TimeEpoch::Rfc868));
    }

    void testTickUnitsAndTruncation() {
        const auto value = DateTime::fromTicks(Seconds{42}, Nanoseconds{999'999'999}, TimeEpoch::Posix).value();

        WITH_CONTEXT(requireTickConversion(value, Nanoseconds{42'999'999'999}, Nanoseconds{999'999'999}));
        WITH_CONTEXT(requireTickConversion(value, Microseconds{42'999'999}, Nanoseconds{999'999'000}));
        WITH_CONTEXT(requireTickConversion(value, Milliseconds{42'999}, Nanoseconds{999'000'000}));
        WITH_CONTEXT(requireTickConversion(value, Seconds{42}, Nanoseconds{}));

        REQUIRE_EQUAL(DateTime::fromTicks(Seconds{42}, TimeEpoch::Posix).value().toTimeT(), std::time_t{42});
    }

    void testTickRangeBoundaries() {
        const auto last = DateTime::last();
        REQUIRE_FALSE(last.toTicks<Nanoseconds>().has_value());
        REQUIRE(last.toTicks<Microseconds>().has_value());
        REQUIRE(last.toTicks<Milliseconds>().has_value());
        REQUIRE(last.toTicks<Seconds>().has_value());
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, last.toTicksOrThrow<Nanoseconds>());

        const auto beforePosixEpoch = DateTime::epoch();
        REQUIRE_FALSE(beforePosixEpoch.toTicks<Seconds>(TimeEpoch::Posix).has_value());
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, beforePosixEpoch.toTicksOrThrow<Seconds>(TimeEpoch::Posix));
    }

    void testSplitTicksAndInvalidValues() {
        const auto value = DateTime::fromTicks(Seconds{84}, Nanoseconds{987'654'321}, TimeEpoch::Posix).value();
        REQUIRE_EQUAL(
            value.toSecondsAndFractions(TimeEpoch::Posix).value(), (std::pair{Seconds{84}, Nanoseconds{987'654'321}}));
        REQUIRE_EQUAL(DateTime::fromTicks(Seconds{84}, Nanoseconds{987'654'321}, TimeEpoch::Posix).value(), value);

        REQUIRE_FALSE(DateTime{}.toSecondsAndFractions().has_value());
        REQUIRE_FALSE(DateTime::epoch().toSecondsAndFractions(TimeEpoch::Posix).has_value());
        REQUIRE_FALSE(DateTime::fromTicks(Seconds{-1}).has_value());
        REQUIRE_FALSE(DateTime::fromTicks(Nanoseconds{-1}).has_value());
        REQUIRE_FALSE(DateTime::fromTicks(Seconds{}, Nanoseconds{-1}).has_value());
        REQUIRE_FALSE(DateTime::fromTicks(Seconds{}, Nanoseconds{1'000'000'000}).has_value());
        REQUIRE_FALSE(DateTime::fromTicks(Seconds{315'569'520'000}).has_value());
        REQUIRE_THROWS_AS(el::err::ParameterError, DateTime::fromTicksOrThrow(Seconds{-1}));
        REQUIRE_THROWS_AS(el::err::ParameterError, DateTime::fromTicksOrThrow(Nanoseconds{-1}));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            DateTime::fromTicksOrThrow(Seconds{}, Nanoseconds{1'000'000'000}, TimeEpoch::Core));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, DateTime::fromTicksOrThrow(Seconds{315'569'520'000}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, DateTime{}.toSecondsAndFractionsOrThrow());
    }

private:
    template <typename tUnit>
    void requireTickConversion(const DateTime &value, const tUnit expectedTicks, const Nanoseconds expectedFractions) {
        const auto ticks = value.toTicks<tUnit>(TimeEpoch::Posix);
        REQUIRE(ticks.has_value());
        REQUIRE_EQUAL(*ticks, expectedTicks);
        REQUIRE_EQUAL(value.toTicksOrThrow<tUnit>(TimeEpoch::Posix), expectedTicks);
        REQUIRE_EQUAL(
            DateTime::fromTicks(expectedTicks, TimeEpoch::Posix).value(),
            DateTime::fromTicks(Seconds{42}, expectedFractions, TimeEpoch::Posix).value());
    }
};
