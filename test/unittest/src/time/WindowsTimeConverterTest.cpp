// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/time/DateTime.hpp>
#include <erbsland/time/impl/WindowsTimeConverter.hpp>
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
        REQUIRE_EQUAL(value.nanosecondFraction(), Nanoseconds{123'456'700});
        REQUIRE_EQUAL(el::time::impl::windows_time_converter::toFileTime(value)->dwLowDateTime, fileTime.dwLowDateTime);
        const auto valueWithNanosecondPrecision =
            DateTime::fromTicks(Seconds{42}, Nanoseconds{123'456'789}, TimeEpoch::Posix).value();
        const auto truncatedFileTime = el::time::impl::windows_time_converter::toFileTime(valueWithNanosecondPrecision);
        REQUIRE(truncatedFileTime.has_value());
        REQUIRE_EQUAL(truncatedFileTime->dwLowDateTime, fileTime.dwLowDateTime);
        REQUIRE_EQUAL(truncatedFileTime->dwHighDateTime, fileTime.dwHighDateTime);
    }
};
