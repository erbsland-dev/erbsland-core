// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/StdFormat.hpp>
#include <erbsland/time/Timestamp.hpp>
#include <erbsland/unit/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <limits>

using namespace el::time;
using namespace el::text::literals;

TESTED_TARGETS(Timestamp)
class TimestampConverterTest final : public el::UnitTest {
public:
    void testCalendarConversions() {
        const auto date = Date::fromYearMonthDay(2026, 10, 1);
        const auto time = Time{Hour{12}, Minute{34}, Second{56}, Nanoseconds{987'654'321}};
        const auto local = DateTime{date, time, Duration{Hours{2}}};
        const auto value = Timestamp::fromDateTimeOrThrow(local);
        REQUIRE_EQUAL(value.date(), local.utcDate());
        REQUIRE_EQUAL(value.time(), local.utcTime());
        REQUIRE_EQUAL(value.toDateTimeOrThrow(), local.toUtc());
        REQUIRE(value.toDateTimeOrThrow().isUtc());
        REQUIRE_EQUAL(Timestamp::fromDateTimeOrThrow(local.toTimeZone(TimeZone{Duration{Hours{-3}}})), value);
        for (const auto timestamp : std::array{Timestamp::first(), value, Timestamp::last()}) {
            REQUIRE_EQUAL(Timestamp::fromDateTimeOrThrow(timestamp.toDateTimeOrThrow()), timestamp);
        }
        REQUIRE_FALSE(Timestamp::fromDateTime(DateTime{}));
        REQUIRE_FALSE(Timestamp{}.toDateTime());
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, Timestamp::fromDateTimeOrThrow(DateTime{}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, Timestamp{}.toDateTimeOrThrow());
    }

    void testIsoParsing() {
        const auto timestamp = Timestamp::fromIsoStringOrThrow("2026-10-01T12:34:56.123456789Z"_el);
        REQUIRE_EQUAL(timestamp.toIsoStringOrThrow(), "2026-10-01T12:34:56.123456789Z"_el);
        REQUIRE_EQUAL(Timestamp::fromIsoStringOrThrow("2026-10-01T14:34:56.123456789+02:00"_el), timestamp);
        REQUIRE_EQUAL(
            Timestamp::fromIsoStringOrThrow("2026-10-01T12:34:56Z"_el).time().nanosecondFraction(), Nanoseconds{});
        REQUIRE_EQUAL(
            Timestamp::fromIsoStringOrThrow("2026-10-01T12:34:56.1Z"_el).time().nanosecondFraction(),
            Nanoseconds{100'000'000});
        for (
            const auto text : std::array{
                "2026-10-01"_el,
                "2026-10-01T12:34Z"_el,
                "2026-10-01T12:34:56"_el,
                "2026-02-30T12:34:56Z"_el,
                "2026-10-01T12:34:56.1234567890Z"_el,
                "2026-10-01T12:34:60Z"_el,
                "0000-01-01T00:00:00+01:00"_el,
                "9999-12-31T23:59:59-01:00"_el}) {
            REQUIRE_FALSE(Timestamp::fromIsoString(text));
            REQUIRE_THROWS_AS(el::err::ParseError, Timestamp::fromIsoStringOrThrow(text));
        }
        REQUIRE_FALSE(Timestamp{}.toIsoString());
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, Timestamp{}.toIsoStringOrThrow());
        REQUIRE_EQUAL(Timestamp::fromIsoStringOrThrow(Timestamp::first().toIsoStringOrThrow()), Timestamp::first());
        REQUIRE_EQUAL(Timestamp::fromIsoStringOrThrow(Timestamp::last().toIsoStringOrThrow()), Timestamp::last());
    }

    void testEpochs() {
        for (const auto epoch : std::array{TimeEpoch::Core, TimeEpoch::Posix, TimeEpoch::Windows, TimeEpoch::Rfc868}) {
            REQUIRE_EQUAL(Timestamp::epoch(epoch).toDateTimeOrThrow(), DateTime::epoch(epoch));
            REQUIRE_EQUAL(Timestamp::fromTicksOrThrow(Seconds{}, epoch), Timestamp::epoch(epoch));
            REQUIRE_EQUAL(Timestamp::epoch(epoch).toTicksOrThrow<Seconds>(epoch), Seconds{});
        }
        REQUIRE_EQUAL(Timestamp::fromTimeT(0), Timestamp::epoch(TimeEpoch::Posix));
        REQUIRE_EQUAL(Timestamp::fromTimeT(42).toTimeT(), std::time_t{42});
        REQUIRE_EQUAL(Timestamp::fromTimeT(-1), Timestamp::epoch(TimeEpoch::Posix) - Seconds{1});
        REQUIRE_FALSE(Timestamp::fromTicks(Seconds{-1}));
    }

    void testSignedFractionalTicks() {
        const auto epoch = Timestamp::epoch(TimeEpoch::Posix);
        const auto before = epoch - Nanoseconds{500'000'001};
        REQUIRE_EQUAL(
            before.toSecondsAndFractionsOrThrow(TimeEpoch::Posix), (std::pair{Seconds{-1}, Nanoseconds{499'999'999}}));
        REQUIRE_EQUAL(before.toTicksOrThrow<Seconds>(TimeEpoch::Posix), Seconds{});
        REQUIRE_EQUAL(before.toTicksOrThrow<Milliseconds>(TimeEpoch::Posix), Milliseconds{-500});
        REQUIRE_EQUAL(before.toTicksOrThrow<Microseconds>(TimeEpoch::Posix), Microseconds{-500'000});
        REQUIRE_EQUAL(before.toTicksOrThrow<Nanoseconds>(TimeEpoch::Posix), Nanoseconds{-500'000'001});
        REQUIRE_EQUAL(Timestamp::fromTicksOrThrow(Seconds{-1}, Nanoseconds{499'999'999}, TimeEpoch::Posix), before);
        REQUIRE_EQUAL(Timestamp::fromTicksOrThrow(Nanoseconds{-500'000'001}, TimeEpoch::Posix), before);
        REQUIRE_EQUAL(Timestamp::fromTicksOrThrow(Milliseconds{-500}, TimeEpoch::Posix), epoch - Milliseconds{500});
        REQUIRE_EQUAL(Timestamp::fromTicksOrThrow(Microseconds{-500'000}, TimeEpoch::Posix), epoch - Milliseconds{500});
    }

    void testTickBounds() {
        REQUIRE_FALSE(Timestamp::last().toTicks<Nanoseconds>());
        REQUIRE(Timestamp::last().toTicks<Microseconds>());
        REQUIRE(Timestamp::last().toTicks<Milliseconds>());
        REQUIRE(Timestamp::last().toTicks<Seconds>());
        REQUIRE_FALSE(Timestamp{}.toTicks<Seconds>());
        REQUIRE_FALSE(Timestamp{}.toSecondsAndFractions());
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, Timestamp::last().toTicksOrThrow<Nanoseconds>());
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, Timestamp{}.toSecondsAndFractionsOrThrow());
        REQUIRE_FALSE(Timestamp::fromTicks(Seconds{std::numeric_limits<int64_t>::max()}, TimeEpoch::Posix));
        REQUIRE_FALSE(Timestamp::fromTicks(Seconds{std::numeric_limits<int64_t>::min()}, TimeEpoch::Posix));
        REQUIRE_FALSE(Timestamp::fromTicks(Seconds{}, Nanoseconds{-1}));
        REQUIRE_FALSE(Timestamp::fromTicks(Seconds{}, Nanoseconds{1'000'000'000}));
        REQUIRE_THROWS_AS(el::err::ParameterError, Timestamp::fromTicksOrThrow(Seconds{}, Nanoseconds{-1}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, Timestamp::fromTicksOrThrow(Seconds{-1}));
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError, Timestamp::fromTicksOrThrow(Seconds{std::numeric_limits<int64_t>::max()}));
        for (const auto ticks : std::array{std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max()}) {
            const auto value = Timestamp::fromTicksOrThrow(Nanoseconds{ticks}, TimeEpoch::Posix);
            REQUIRE_EQUAL(value.toTicksOrThrow<Nanoseconds>(TimeEpoch::Posix), Nanoseconds{ticks});
        }
    }
};
