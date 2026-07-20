// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/time/all.hpp>
#include <erbsland/time/StdFormatForTime.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>

using namespace el::time;

using namespace el::text::literals;

using el::text::String;
using el::text::StringConverter;
using el::time::Seconds;

TESTED_TARGETS(time TimeZone DateTime)
class TimeZoneTransitionTest final : public el::UnitTest {
    struct Case final {
        String name;
        int month;
        int day;
        int hour;
        int minute;
        Seconds offset;
        const char *abbreviation;
    };

public:
    void testRepresentativeZones2026() {

        const auto cases = std::array{
            Case{"Europe/Zurich"_el, 1, 1, 13, 0, Seconds{3600}, "CET"},
            Case{"Europe/Zurich"_el, 7, 1, 14, 0, Seconds{7200}, "CEST"},
            Case{"America/New_York"_el, 1, 1, 7, 0, Seconds{-18000}, "EST"},
            Case{"America/New_York"_el, 7, 1, 8, 0, Seconds{-14400}, "EDT"},
            Case{"Asia/Tokyo"_el, 1, 1, 21, 0, Seconds{32400}, "JST"},
            Case{"Asia/Tokyo"_el, 7, 1, 21, 0, Seconds{32400}, "JST"},
            Case{"Australia/Lord_Howe"_el, 1, 1, 23, 0, Seconds{39600}, "+11"},
            Case{"Australia/Lord_Howe"_el, 7, 1, 22, 30, Seconds{37800}, "+1030"},
            Case{"Pacific/Chatham"_el, 1, 2, 1, 45, Seconds{49500}, "+1345"},
            Case{"Pacific/Chatham"_el, 7, 2, 0, 45, Seconds{45900}, "+1245"},
            Case{"Etc/GMT+5"_el, 1, 1, 7, 0, Seconds{-18000}, "-05"},
        };
        for (const auto &testCase : cases) {
            const auto zone = TimeZone::fromNameOrThrow(testCase.name);
            const auto utc = DateTime{Date::fromYearMonthDay(2026, testCase.month, 1), Time{Hour{12}, Minute{0}}};
            const auto local = utc.toTimeZone(zone);
            REQUIRE_EQUAL(local.day(), Day{static_cast<int8_t>(testCase.day)});
            REQUIRE_EQUAL(local.hour(), Hour{static_cast<int8_t>(testCase.hour)});
            REQUIRE_EQUAL(local.minute(), Minute{static_cast<int8_t>(testCase.minute)});
            REQUIRE_EQUAL(local.timeOffset(), Duration{testCase.offset});
            REQUIRE_EQUAL(StringConverter{local.timeZoneAbbreviation()}.toStdString(), testCase.abbreviation);
        }
    }

    void testHistoricalZurichTransition() {

        const auto zurich = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        const auto before = DateTime{Date::fromYearMonthDay(1981, 3, 29), Time{Hour{0}, Minute{59}}}.toTimeZone(zurich);
        const auto after = DateTime{Date::fromYearMonthDay(1981, 3, 29), Time{Hour{1}, Minute{0}}}.toTimeZone(zurich);
        REQUIRE_EQUAL(before.timeOffset(), Duration{Seconds{3600}});
        REQUIRE_EQUAL(StringConverter{before.timeZoneAbbreviation()}.toStdString(), "CET");
        REQUIRE_EQUAL(after.timeOffset(), Duration{Seconds{7200}});
        REQUIRE_EQUAL(StringConverter{after.timeZoneAbbreviation()}.toStdString(), "CEST");
    }
};
