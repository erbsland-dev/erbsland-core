// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/time/impl/WindowsTimeConverter.hpp>
#include <erbsland/time/StdFormat.hpp>
#include <erbsland/time/Timestamp.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>

using namespace el::time;

TESTED_TARGETS(WindowsTimeConverter)
class WindowsTimeConverterTest final : public el::UnitTest {
public:
    void testFileTimeConversion() {
        constexpr auto cPosixEpochTicks = std::uint64_t{116'444'736'000'000'000ULL};
        constexpr auto cFractionTicks = std::uint64_t{1'234'567ULL};

        auto fileTime = FILETIME{};
        const auto ticks = cPosixEpochTicks + 42U * 10'000'000U + cFractionTicks;
        fileTime.dwLowDateTime = static_cast<DWORD>(ticks & 0xffffffffULL);
        fileTime.dwHighDateTime = static_cast<DWORD>(ticks >> 32U);
        const auto value = el::time::impl::windows_time_converter::fromFileTime(fileTime);
        REQUIRE_EQUAL(value.toSecondsAndFractions(TimeEpoch::Posix)->first, Seconds{42});
        REQUIRE_EQUAL(value.time().nanosecondFraction(), Nanoseconds{123'456'700});
        REQUIRE_EQUAL(el::time::impl::windows_time_converter::toFileTime(value)->dwLowDateTime, fileTime.dwLowDateTime);
        const auto valueWithNanosecondPrecision =
            Timestamp::fromTicks(Seconds{42}, Nanoseconds{123'456'789}, TimeEpoch::Posix).value();
        const auto truncatedFileTime = el::time::impl::windows_time_converter::toFileTime(valueWithNanosecondPrecision);
        REQUIRE(truncatedFileTime.has_value());
        REQUIRE_EQUAL(truncatedFileTime->dwLowDateTime, fileTime.dwLowDateTime);
        REQUIRE_EQUAL(truncatedFileTime->dwHighDateTime, fileTime.dwHighDateTime);
    }
    void testTimestampConversion() {
        constexpr auto cPosixEpochTicks = uint64_t{116'444'736'000'000'000ULL};
        const auto ticks = cPosixEpochTicks + 42ULL * 10'000'000ULL + 1'234'567ULL;
        const auto fileTime = FILETIME{static_cast<DWORD>(ticks), static_cast<DWORD>(ticks >> 32U)};
        const auto value = impl::windows_time_converter::fromFileTime(fileTime);
        REQUIRE_EQUAL(value, Timestamp::fromTicksOrThrow(Seconds{42}, Nanoseconds{123'456'700}, TimeEpoch::Posix));
        const auto output = impl::windows_time_converter::toFileTime(value + Nanoseconds{89});
        REQUIRE(output);
        REQUIRE_EQUAL(output->dwLowDateTime, fileTime.dwLowDateTime);
        REQUIRE_EQUAL(output->dwHighDateTime, fileTime.dwHighDateTime);
        REQUIRE_FALSE(impl::windows_time_converter::toFileTime(Timestamp{}));
        REQUIRE_FALSE(impl::windows_time_converter::toFileTime(Timestamp::first()));
        REQUIRE_FALSE(impl::windows_time_converter::fromFileTime(FILETIME{0xffffffffU, 0xffffffffU}).isValid());
        const auto last = impl::windows_time_converter::toFileTime(Timestamp::last());
        REQUIRE(last);
        REQUIRE_EQUAL(impl::windows_time_converter::fromFileTime(*last), Timestamp::last() - Nanoseconds{99});
    }
};
