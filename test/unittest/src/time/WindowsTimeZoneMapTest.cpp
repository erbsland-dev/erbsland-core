// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/time/StdFormatForTime.hpp>
#include <erbsland/time/TimeZone.hpp>
#include <erbsland/time/tz/impl/WindowsTimeZoneMap.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(time WindowsTimeZoneMap)
class WindowsTimeZoneMapTest final : public el::UnitTest {
public:
    void testKnownAndUnknownNames() {
        const auto id = el::time::tz::impl::timeZoneIdFromWindowsName(L"Central Europe Standard Time");
        REQUIRE(id.has_value());
        REQUIRE_EQUAL(el::time::TimeZone{id.value()}.name(), "Europe/Budapest"_el);
        REQUIRE_FALSE(el::time::tz::impl::timeZoneIdFromWindowsName(L"Unknown Time Zone").has_value());
    }
};
