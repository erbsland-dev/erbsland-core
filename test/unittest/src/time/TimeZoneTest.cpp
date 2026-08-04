// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/time/all.hpp>
#include <erbsland/time/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <vector>

using namespace el::time;

using namespace el::text::literals;

using el::text::StringConverter;
using el::text::StringEditor;
using el::unit::ItemCount;
using el::unit::Version;

TESTED_TARGETS(time TimeZone TimeZoneId)
class TimeZoneTest final : public el::UnitTest {
public:
    void testUtcAliasesAndInvalidNames() {

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
            const auto local = utc.toTimeZone(*zone);
            REQUIRE_EQUAL(local.timeOffset(), Duration{});
        }
        REQUIRE(TimeZone::fromNameOrThrow("Factory"_el).isUtc());
        REQUIRE_FALSE(TimeZone::fromName("Mars/Olympus"_el).has_value());
        REQUIRE_THROWS(TimeZone::fromNameOrThrow("Mars/Olympus"_el));
    }

    void testFixedOffsetParsing() {

        struct ValidCase {
            StringEditor name;
            Seconds offset;
        };
        const auto validCases = std::vector<ValidCase>{
            {StringEditor{"+1"}, Seconds{3600}},
            {StringEditor{"+01"}, Seconds{3600}},
            {StringEditor{"+0130"}, Seconds{5400}},
            {StringEditor{"-0330"}, Seconds{-12600}},
            {StringEditor{"+01:30"}, Seconds{5400}},
            {StringEditor{"-03:30"}, Seconds{-12600}},
            {StringEditor{"+01:02:03"}, Seconds{3723}},
            {StringEditor{"UTC+1"}, Seconds{3600}},
            {StringEditor{"utc+01:30"}, Seconds{5400}},
            {StringEditor{"GMT-0330"}, Seconds{-12600}},
            {StringEditor{"gmt+01:02:03"}, Seconds{3723}},
            {StringEditor{"UtC+1"}, Seconds{3600}},
            {StringEditor{"uTc+01:30"}, Seconds{5400}},
            {StringEditor{"GmT-0330"}, Seconds{-12600}},
            {StringEditor{"gMt+01:02:03"}, Seconds{3723}},
            {StringEditor{"+12:01"}, Seconds{12 * 3600 + 60}},
            {StringEditor{"+13:00"}, Seconds{13 * 3600}},
            {StringEditor{"+14:00"}, Seconds{14 * 3600}},
            {StringEditor{"+23:59:59"}, Seconds{86'399}},
            {StringEditor{"-23:59:59"}, Seconds{-86'399}},
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

    void testFullDayNormalization() {

        const auto positiveThirteen = TimeZone{Hours{13}};
        REQUIRE_EQUAL(positiveThirteen.staticOffset(), Duration{Hours{13}});
        const auto negativeThirteen = TimeZone{Hours{-13}};
        REQUIRE_EQUAL(negativeThirteen.staticOffset(), Duration{Hours{-13}});
        const auto positiveThirtySeven = TimeZone{Hours{37}};
        REQUIRE_EQUAL(positiveThirtySeven.staticOffset(), Duration{Hours{23}});
        const auto negativeThirtySeven = TimeZone{Hours{-37}};
        REQUIRE_EQUAL(negativeThirtySeven.staticOffset(), Duration{Hours{-23}});

        const auto dateTime =
            DateTime{Date::fromYearMonthDay(2026, 1, 1), Time{Hour{1}, Minute{0}}, TimeZone{Hours{14}}};
        REQUIRE_EQUAL(dateTime.utcDate(), Date::fromYearMonthDay(2025, 12, 31));
        REQUIRE_EQUAL(dateTime.utcTime(), (Time{Hour{11}, Minute{0}}));
        REQUIRE_EQUAL(dateTime.timeOffset(), Duration{Hours{14}});
    }

    void testGeneratedNamesAndDatabaseVersion() {

        const auto databaseVersion = TimeZone::databaseVersion();
        REQUIRE_EQUAL(databaseVersion, (Version{1, 2026, 2}));
        const auto names = TimeZone::names();
        REQUIRE_GREATER(names.count(), ItemCount{500});
        REQUIRE(names.contains(StringEditor{"Europe/Zurich"}));
        REQUIRE(names.contains(StringEditor{"America/New_York"}));
        REQUIRE(names.contains(StringEditor{"Etc/GMT+1"}));
        REQUIRE(names.contains(StringEditor{"US/Eastern"}));

        const auto zurich = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        const auto zurichAgain = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        REQUIRE(zurich.isNamed());
        REQUIRE_EQUAL(zurich.id(), zurichAgain.id());
        const auto zurichName = StringConverter{zurich.name()}.toStdString();
        REQUIRE_EQUAL(zurichName, "Europe/Zurich");

        const auto eastern = TimeZone::fromNameOrThrow("US/Eastern"_el);
        REQUIRE(eastern.isNamed());
        const auto easternName = StringConverter{eastern.name()}.toStdString();
        REQUIRE_EQUAL(easternName, "America/New_York");
    }

    void testGeneratedEtcNamesTakePrecedence() {

        const auto generated = TimeZone::fromNameOrThrow("Etc/GMT+1"_el);
        const auto utc = DateTime{Date::fromYearMonthDay(2026, 1, 1), Time{Hour{12}, Minute{0}}};
        const auto local = utc.toTimeZone(generated);
        REQUIRE(generated.isNamed());
        REQUIRE_EQUAL(local.timeOffset(), Duration{Seconds{-3600}});
        REQUIRE_EQUAL(local.hour(), Hour{11});
    }
};
