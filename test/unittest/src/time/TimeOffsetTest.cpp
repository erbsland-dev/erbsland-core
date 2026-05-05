// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/time/all.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(time TimeOffset)
class TimeOffsetTest final : public el::UnitTest {
public:
    void testUtcAndFixedOffsets() {
        using namespace el::time;

        const auto utc = tz::TimeOffset{};
        REQUIRE(utc.isUtc());
        REQUIRE_FALSE(utc.isStaticOffset());
        REQUIRE_FALSE(utc.isZone());
        REQUIRE_FALSE(utc.isDst());
        REQUIRE_EQUAL(utc.offset(), Seconds{0});

        const auto fixed = tz::TimeOffset{Seconds{19800}};
        REQUIRE_FALSE(fixed.isUtc());
        REQUIRE(fixed.isStaticOffset());
        REQUIRE_FALSE(fixed.isZone());
        REQUIRE_EQUAL(fixed.offset(), Seconds{19800});
    }

    void testNamedOffsetStorage() {
        using namespace el::time;

        const auto offset = tz::TimeOffset{Seconds{7200}, true, TimeZoneId{321}, 7};
        REQUIRE_FALSE(offset.isUtc());
        REQUIRE_FALSE(offset.isStaticOffset());
        REQUIRE(offset.isZone());
        REQUIRE(offset.isDst());
        REQUIRE_EQUAL(offset.offset(), Seconds{7200});
        REQUIRE_EQUAL(offset.zoneId(), TimeZoneId{321});
        REQUIRE_EQUAL(offset.abbreviationId(), 7);
    }
};
