// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/time/all.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <vector>

using el::text::String;
using el::text::StringConverter;
using el::unit::ElementCount;
using el::unit::Version;

TESTED_TARGETS(time TimeZone TimeZoneId)
class TimeZoneTest final : public el::UnitTest {
public:
    void testUtcAliasesAndInvalidNames() {
        using namespace el::text::literals;
        using namespace el::time;

        for (
            const auto name : {
                "UTC"_el,
                "utc"_el,
                "UtC"_el,
                "GMT"_el,
                "gMt"_el,
                "Z"_el,
                "z"_el,
                "Zulu"_el,
                "zUlU"_el,
                "Factory"_el,
                "fAcToRy"_el,
            }) {
            const auto zone = TimeZone::fromName(name);
            REQUIRE(zone.has_value());
            const auto utc = DateTime{Date::fromYearMonthDay(2026, 1, 1), Time{Hour{12}, Minute{0}}};
            REQUIRE_EQUAL(utc.toTimeZone(*zone).timeOffset(), Duration{});
        }
        REQUIRE(TimeZone::fromNameOrThrow("Factory"_el).isUtc());
        REQUIRE_FALSE(TimeZone::fromName("Mars/Olympus"_el).has_value());
        REQUIRE_THROWS(TimeZone::fromNameOrThrow("Mars/Olympus"_el));
    }

    void testFixedOffsetParsing() {
        using namespace el::text::literals;
        using namespace el::time;

        struct ValidCase {
            String name;
            Seconds offset;
        };
        const auto validCases = std::vector<ValidCase>{
            {String{"+1"}, Seconds{3600}},
            {String{"+01"}, Seconds{3600}},
            {String{"+0130"}, Seconds{5400}},
            {String{"-0330"}, Seconds{-12600}},
            {String{"+01:30"}, Seconds{5400}},
            {String{"-03:30"}, Seconds{-12600}},
            {String{"+01:02:03"}, Seconds{3723}},
            {String{"UTC+1"}, Seconds{3600}},
            {String{"utc+01:30"}, Seconds{5400}},
            {String{"GMT-0330"}, Seconds{-12600}},
            {String{"gmt+01:02:03"}, Seconds{3723}},
            {String{"UtC+1"}, Seconds{3600}},
            {String{"uTc+01:30"}, Seconds{5400}},
            {String{"GmT-0330"}, Seconds{-12600}},
            {String{"gMt+01:02:03"}, Seconds{3723}},
        };
        for (const auto &testCase : validCases) {
            const auto zone = TimeZone::fromNameOrThrow(testCase.name);
            REQUIRE(zone.isStaticOffset());
            REQUIRE_EQUAL(zone.staticOffset(), Duration{testCase.offset});
        }

        for (
            const auto name : {
                "+"_el,
                "-"_el,
                "+1:"_el,
                "+1:2"_el,
                "+123"_el,
                "+012345"_el,
                "+24"_el,
                "+23:60"_el,
                "+23:59:60"_el,
                "UTC+"_el,
                "uTc+"_el,
                "GMT+010203"_el,
                "gMt+010203"_el,
            }) {
            REQUIRE_FALSE(TimeZone::fromName(name).has_value());
        }
    }

    void testGeneratedNamesAndDatabaseVersion() {
        using namespace el::text::literals;
        using namespace el::time;

        REQUIRE_EQUAL(TimeZone::databaseVersion(), (Version{1, 2026, 2}));
        const auto names = TimeZone::names();
        REQUIRE(names.count() > ElementCount{500});
        REQUIRE(names.contains(String{"Europe/Zurich"}));
        REQUIRE(names.contains(String{"America/New_York"}));
        REQUIRE(names.contains(String{"Etc/GMT+1"}));
        REQUIRE(names.contains(String{"US/Eastern"}));

        const auto zurich = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        const auto zurichAgain = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        REQUIRE(zurich.isNamed());
        REQUIRE_EQUAL(zurich.id(), zurichAgain.id());
        REQUIRE_EQUAL(StringConverter{zurich.name()}.toStdString(), "Europe/Zurich");

        const auto eastern = TimeZone::fromNameOrThrow("US/Eastern"_el);
        REQUIRE(eastern.isNamed());
        REQUIRE_EQUAL(StringConverter{eastern.name()}.toStdString(), "America/New_York");
    }

    void testGeneratedEtcNamesTakePrecedence() {
        using namespace el::text::literals;
        using namespace el::time;

        const auto generated = TimeZone::fromNameOrThrow("Etc/GMT+1"_el);
        const auto utc = DateTime{Date::fromYearMonthDay(2026, 1, 1), Time{Hour{12}, Minute{0}}};
        const auto local = utc.toTimeZone(generated);
        REQUIRE(generated.isNamed());
        REQUIRE_EQUAL(local.timeOffset(), Duration{Seconds{-3600}});
        REQUIRE_EQUAL(local.hour(), Hour{11});
    }
};
