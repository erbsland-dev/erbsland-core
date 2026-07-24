// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/time/impl/PosixTimeConverter.hpp>
#include <erbsland/time/impl/WindowsTimeConverter.hpp>
#include <erbsland/time/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <ctime>

using namespace el::time;

TESTED_TARGETS(DateTime PosixTimeConverter WindowsTimeConverter)
class TimeConverterTest final : public el::UnitTest {
public:
    void testPosixTimeConversion() {
        const auto epoch = impl::PosixTimeConverter::fromPosixTime(Seconds{0});
        REQUIRE(epoch.isValid());
        REQUIRE_EQUAL(epoch, DateTime::posixEpoch());

        const auto value = impl::PosixTimeConverter::fromPosixTime(Seconds{42}, Nanoseconds{123'456'789});
        REQUIRE(value.isValid());
        REQUIRE_EQUAL(value.toTimeT(), std::time_t{42});
        REQUIRE_EQUAL(value.nanosecondFraction(), Nanoseconds{123'456'789});

        const auto invalidFraction = impl::PosixTimeConverter::fromPosixTime(Seconds{42}, Nanoseconds{1'000'000'000});
        REQUIRE_FALSE(invalidFraction.isValid());
    }

    void testTimespecConversion() {
        auto timeSpec = timespec{};
        timeSpec.tv_sec = 84;
        timeSpec.tv_nsec = 987'654'321;

        const auto value = impl::PosixTimeConverter::fromTimespec(timeSpec);
        REQUIRE(value.isValid());
        REQUIRE_EQUAL(value.toTimeT(), std::time_t{84});
        REQUIRE_EQUAL(value.nanosecondFraction(), Nanoseconds{987'654'321});
    }

    void testWindowsFileTimeTickConversion() {
        constexpr auto windowsEpochTicks = std::uint64_t{0ULL};
        constexpr auto posixEpochTicks = std::uint64_t{116'444'736'000'000'000ULL};
        constexpr auto ticksPerSecond = std::uint64_t{10'000'000ULL};
        constexpr auto fractionTicks = std::uint64_t{1'234'567ULL};

        const auto windowsEpoch = impl::WindowsTimeConverter::fromFileTimeTicks(windowsEpochTicks);
        REQUIRE(windowsEpoch.isValid());
        REQUIRE_EQUAL(windowsEpoch.utcDate(), Date::fromYearMonthDay(1601, 1, 1));
        REQUIRE_EQUAL(impl::WindowsTimeConverter::toFileTimeTicks(windowsEpoch).value(), windowsEpochTicks);

        const auto epoch = impl::WindowsTimeConverter::fromFileTimeTicks(posixEpochTicks);
        REQUIRE(epoch.isValid());
        REQUIRE_EQUAL(epoch, DateTime::posixEpoch());
        REQUIRE_EQUAL(impl::WindowsTimeConverter::toFileTimeTicks(epoch).value(), posixEpochTicks);

        const auto value =
            impl::WindowsTimeConverter::fromFileTimeTicks(posixEpochTicks + 42U * ticksPerSecond + fractionTicks);
        REQUIRE(value.isValid());
        REQUIRE_EQUAL(value.toTimeT(), std::time_t{42});
        REQUIRE_EQUAL(value.nanosecondFraction(), Nanoseconds{123'456'700});
        REQUIRE_EQUAL(
            impl::WindowsTimeConverter::toFileTimeTicks(value).value(),
            posixEpochTicks + 42U * ticksPerSecond + fractionTicks);

        REQUIRE_FALSE(impl::WindowsTimeConverter::toFileTimeTicks(DateTime{}).has_value());
        REQUIRE_FALSE(impl::WindowsTimeConverter::toFileTimeTicks(DateTime::epoch()).has_value());
    }
};
