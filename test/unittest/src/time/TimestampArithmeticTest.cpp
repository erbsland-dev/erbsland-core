// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/OverflowError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/time/StdFormat.hpp>
#include <erbsland/time/Timestamp.hpp>
#include <erbsland/unit/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <limits>

using namespace el::time;

TESTED_TARGETS(Timestamp TimestampAmount)
class TimestampArithmeticTest final : public el::UnitTest {
public:
    void testFixedAmounts() {
        WITH_CONTEXT(checkAmount(Seconds{1}, 1'000'000'000));
        WITH_CONTEXT(checkAmount(Milliseconds{1}, 1'000'000));
        WITH_CONTEXT(checkAmount(Microseconds{1}, 1'000));
        WITH_CONTEXT(checkAmount(Nanoseconds{1}, 1));
        WITH_CONTEXT(checkAmount(Duration{Seconds{1}}, 1'000'000'000));
        WITH_CONTEXT(checkAmount(TimeDelta{Nanoseconds{1}}, 1));
    }

    void testMidnightAndSaturation() {
        const auto midnight = Timestamp::fromRawValueOrThrow(42, 0);
        REQUIRE_EQUAL(midnight - Nanoseconds{1}, Timestamp::fromRawValueOrThrow(41, 86'399'999'999'999));
        REQUIRE_EQUAL((midnight - Nanoseconds{1}) + Nanoseconds{1}, midnight);
        REQUIRE_EQUAL(midnight.added(Seconds{-1}), midnight.subtracted(Seconds{1}));
        REQUIRE_EQUAL(Timestamp::first().subtracted(Nanoseconds{1}), Timestamp::first());
        REQUIRE_EQUAL(Timestamp::last().added(Nanoseconds{1}), Timestamp::last());
        REQUIRE(Timestamp::first().wouldSubtractSaturate(Nanoseconds{1}));
        REQUIRE(Timestamp::last().wouldAddSaturate(Nanoseconds{1}));
        REQUIRE_FALSE(Timestamp::last().wouldAddSaturate(Nanoseconds{}));
        auto value = Timestamp::last();
        REQUIRE_THROWS_AS(el::err::OverflowError, value.addOrThrow(Nanoseconds{1}));
        REQUIRE_EQUAL(value, Timestamp::last());
        REQUIRE_THROWS_AS(el::err::OverflowError, Timestamp::first().subtractedOrThrow(Nanoseconds{1}));
    }

    void testOperandExtremes() {
        WITH_CONTEXT(checkExtremes<Seconds>());
        WITH_CONTEXT(checkExtremes<Milliseconds>());
        WITH_CONTEXT(checkExtremes<Microseconds>());
        WITH_CONTEXT(checkExtremes<Nanoseconds>());
        const auto base = Timestamp::epoch(TimeEpoch::Posix);
        REQUIRE_EQUAL(base + Duration{Seconds{std::numeric_limits<int64_t>::max()}}, Timestamp::last());
        REQUIRE_EQUAL(base - Duration{Seconds{std::numeric_limits<int64_t>::min()}}, Timestamp::last());
        REQUIRE_EQUAL(
            base + TimeDelta{Nanoseconds{std::numeric_limits<int64_t>::min()}},
            base.added(Nanoseconds{std::numeric_limits<int64_t>::min()}));
    }

    void testSignedDistances() {
        const auto midnight = Timestamp::fromRawValueOrThrow(100, 0);
        const auto before = midnight - Nanoseconds{1};
        const auto after = midnight + Nanoseconds{1};
        REQUIRE_EQUAL(before.nanosecondsTo(after), Nanoseconds{2});
        REQUIRE_EQUAL(after.nanosecondsTo(before), Nanoseconds{-2});
        REQUIRE_EQUAL(before.secondsTo(after), Seconds{});
        REQUIRE_EQUAL(after.secondsTo(before), Seconds{});
        REQUIRE_EQUAL(midnight.millisecondsTo(midnight + Microseconds{1999}), Milliseconds{1});
        REQUIRE_EQUAL((midnight + Microseconds{1999}).millisecondsTo(midnight), Milliseconds{-1});
        REQUIRE_EQUAL(midnight.microsecondsTo(midnight + Nanoseconds{1999}), Microseconds{1});
        REQUIRE_EQUAL(midnight.secondsTo(midnight + Nanoseconds{1'999'999'999}), Seconds{1});
        REQUIRE_EQUAL(after - before, TimeDelta{Nanoseconds{2}});
        REQUIRE_EQUAL(before.durationTo(after), Duration{});
        REQUIRE_EQUAL(before.timeDeltaToOrThrow(after), TimeDelta{Nanoseconds{2}});
    }

    void testDistanceOverflow() {
        constexpr auto cMaximum = std::numeric_limits<int64_t>::max();
        constexpr auto cMinimum = std::numeric_limits<int64_t>::min();
        const auto base = Timestamp::epoch(TimeEpoch::Posix);
        const auto maximum = base + Nanoseconds{cMaximum};
        const auto minimum = base + Nanoseconds{cMinimum};
        REQUIRE_EQUAL(base.nanosecondsToOrThrow(maximum), Nanoseconds{cMaximum});
        REQUIRE_EQUAL(base.nanosecondsToOrThrow(minimum), Nanoseconds{cMinimum});
        REQUIRE_FALSE(base.wouldNanosecondsToSaturate(maximum));
        REQUIRE_FALSE(base.wouldNanosecondsToSaturate(minimum));
        REQUIRE(base.wouldNanosecondsToSaturate(maximum + Nanoseconds{1}));
        REQUIRE(base.wouldNanosecondsToSaturate(minimum - Nanoseconds{1}));
        REQUIRE_EQUAL(base.nanosecondsTo(maximum + Nanoseconds{1}), Nanoseconds{cMaximum});
        REQUIRE_EQUAL(base.nanosecondsTo(minimum - Nanoseconds{1}), Nanoseconds{cMinimum});
        REQUIRE_THROWS_AS(el::err::OverflowError, base.nanosecondsToOrThrow(maximum + Nanoseconds{1}));
        REQUIRE(Timestamp::first().wouldTimeDeltaToSaturate(Timestamp::last()));
        REQUIRE_FALSE(Timestamp::first().wouldSecondsToSaturate(Timestamp::last()));
        REQUIRE_FALSE(Timestamp::first().wouldMillisecondsToSaturate(Timestamp::last()));
        REQUIRE_FALSE(Timestamp::first().wouldMicrosecondsToSaturate(Timestamp::last()));
        REQUIRE_FALSE(Timestamp::first().wouldDurationToSaturate(Timestamp::last()));
    }

    void testInvalidOperands() {
        REQUIRE_EQUAL(Timestamp{}.addedOrThrow(Seconds{std::numeric_limits<int64_t>::max()}), Timestamp{});
        REQUIRE_EQUAL(Timestamp{}.subtractedOrThrow(Nanoseconds{std::numeric_limits<int64_t>::min()}), Timestamp{});
        REQUIRE_FALSE(Timestamp{}.wouldAddSaturate(Seconds{std::numeric_limits<int64_t>::max()}));
        REQUIRE_EQUAL(Timestamp{}.nanosecondsTo(Timestamp::last()), Nanoseconds{});
        REQUIRE_EQUAL(Timestamp::last().nanosecondsTo(Timestamp{}), Nanoseconds{});
        REQUIRE_FALSE(Timestamp{}.wouldNanosecondsToSaturate(Timestamp::last()));
        REQUIRE_THROWS_AS(el::err::ParameterError, Timestamp{}.secondsToOrThrow(Timestamp::first()));
        REQUIRE_THROWS_AS(el::err::ParameterError, Timestamp::first().timeDeltaToOrThrow(Timestamp{}));
    }

private:
    template <el::time::impl::TimestampAmount T>
    void checkAmount(T amount, int64_t expectedNanoseconds) {
        const auto base = Timestamp::fromRawValueOrThrow(42, 0);
        const auto expected = Timestamp::fromRawValueOrThrow(42, expectedNanoseconds);
        REQUIRE_EQUAL(base + amount, expected);
        REQUIRE_EQUAL(expected - amount, base);
        REQUIRE_EQUAL(base.addedOrThrow(amount), expected);
        REQUIRE_EQUAL(expected.subtractedOrThrow(amount), base);
        auto value = base;
        value += amount;
        REQUIRE_EQUAL(value, expected);
        value -= amount;
        REQUIRE_EQUAL(value, base);
        value.add(amount);
        value.subtract(amount);
        REQUIRE_EQUAL(value, base);
        value.addOrThrow(amount);
        value.subtractOrThrow(amount);
        REQUIRE_EQUAL(value, base);
    }

    template <el::time::impl::DateTimeTickUnit T>
    void checkExtremes() {
        constexpr auto cMinimum = std::numeric_limits<int64_t>::min();
        constexpr auto cMaximum = std::numeric_limits<int64_t>::max();
        const auto base = Timestamp::epoch(TimeEpoch::Posix);
        if constexpr (std::same_as<T, Nanoseconds>) {
            REQUIRE_EQUAL((base + T{cMaximum}) - T{cMaximum}, base);
            REQUIRE_EQUAL((base - T{cMinimum}) + T{cMinimum}, base);
        } else {
            REQUIRE_EQUAL(base + T{cMaximum}, Timestamp::last());
            REQUIRE_EQUAL(base - T{cMinimum}, Timestamp::last());
            REQUIRE_EQUAL(base + T{cMinimum}, Timestamp::first());
            REQUIRE_EQUAL(base - T{cMaximum}, Timestamp::first());
            REQUIRE_THROWS_AS(el::err::OverflowError, base.addedOrThrow(T{cMaximum}));
        }
    }
};
