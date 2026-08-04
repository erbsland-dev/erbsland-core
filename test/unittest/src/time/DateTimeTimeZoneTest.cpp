// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/time/all.hpp>
#include <erbsland/time/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::time;

using namespace el::text::literals;

using el::text::StringConverter;

TESTED_TARGETS(time DateTime TimeZone)
class DateTimeTimeZoneTest final : public el::UnitTest {
public:
    void testUtcConversionAndAbbreviationLookup() {

        const auto zurich = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        const auto utc = DateTime{Date::fromYearMonthDay(2026, 7, 1), Time{Hour{12}, Minute{0}}};
        const auto local = utc.toTimeZone(zurich);
        REQUIRE_EQUAL(local.toUtc(), utc);
        REQUIRE_EQUAL(local.timeOffset(), Duration{Seconds{7200}});
        REQUIRE_EQUAL(StringConverter{local.timeZone().name()}.toStdString(), "Europe/Zurich");
        REQUIRE_EQUAL(StringConverter{local.timeZoneAbbreviation()}.toStdString(), "CEST");
    }

    void testNamedLocalConstructionGapAndFold() {

        const auto zurich = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        const auto gap = DateTime{Date::fromYearMonthDay(2026, 3, 29), Time{Hour{2}, Minute{30}}, zurich};
        REQUIRE_EQUAL(gap.timeOffset(), Duration{Seconds{7200}});
        REQUIRE_EQUAL(gap.hour(), Hour{2});
        REQUIRE_EQUAL(gap.minute(), Minute{30});

        const auto foldDate = Date::fromYearMonthDay(2026, 10, 25);
        const auto first = DateTime{foldDate, Time{Hour{2}, Minute{30}}, zurich, TimeOccurrenceInFold::First};
        const auto second = DateTime{foldDate, Time{Hour{2}, Minute{30}}, zurich, TimeOccurrenceInFold::Second};
        REQUIRE_EQUAL(first.timeOffset(), Duration{Seconds{7200}});
        REQUIRE_EQUAL(second.timeOffset(), Duration{Seconds{3600}});
        REQUIRE_EQUAL(StringConverter{first.timeZoneAbbreviation()}.toStdString(), "CEST");
        REQUIRE_EQUAL(StringConverter{second.timeZoneAbbreviation()}.toStdString(), "CET");
        REQUIRE_EQUAL(first.durationTo(second), Duration{Seconds{3600}});
    }

    void testIsoParsingWithOffsetsAndNamedLocalZone() {

        const auto offset = DateTime::fromIsoString("2026-07-01T14:00:00+02:00"_el);
        REQUIRE(offset.isValid());
        REQUIRE_EQUAL(offset.utcDate(), Date::fromYearMonthDay(2026, 7, 1));
        REQUIRE_EQUAL(offset.utcTime(), (Time{Hour{12}, Minute{0}}));

        const auto zurich = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        const auto local = DateTime::fromIsoString("2026-07-01T14:00:00"_el, zurich);
        REQUIRE(local.isValid());
        REQUIRE_EQUAL(local.utcDate(), Date::fromYearMonthDay(2026, 7, 1));
        REQUIRE_EQUAL(local.utcTime(), (Time{Hour{12}, Minute{0}}));
        REQUIRE_EQUAL(StringConverter{local.timeZoneAbbreviation()}.toStdString(), "CEST");

        REQUIRE_FALSE(DateTime::fromIsoString("2026-07-01T14:00:00+02:00"_el, zurich).isValid());
    }

    void testDateTimeOperatorSubtractionSign() {

        const auto earlier = DateTime{Date::fromYearMonthDay(2026, 5, 20), Time{Hour{12}, Minute{0}}};
        const auto later = DateTime{Date::fromYearMonthDay(2026, 5, 20), Time{Hour{12}, Minute{1}, Second{30}}};

        REQUIRE_EQUAL(later - earlier, Duration{Seconds{90}});
        REQUIRE_EQUAL(earlier - later, Duration{Seconds{-90}});
    }

    void testStrictIsoParsing() {

        REQUIRE_FALSE(DateTime::fromIsoString("2026-"_el, DateTimePrecision::Year).isValid());
        REQUIRE_FALSE(DateTime::fromIsoString("2026-05-"_el, DateTimePrecision::Month).isValid());
        REQUIRE_FALSE(DateTime::fromIsoString("2026-05-20T12:"_el, DateTimePrecision::Hour).isValid());
        REQUIRE_FALSE(DateTime::fromIsoString("2026-05-20T12:30:00."_el).isValid());
        REQUIRE_FALSE(DateTime::fromIsoString("2026-05-20T12:30:00+99:99"_el).isValid());
        REQUIRE_FALSE(DateTime::fromIsoString("2026-05-20T24:00:00"_el).isValid());
        REQUIRE_FALSE(DateTime::fromIsoString("2026-05-20Z"_el, DateTimePrecision::Day).isValid());

        const auto offsetWithSeconds = DateTime::fromIsoString("2026-05-20T12:30:00+01:02:03"_el);
        REQUIRE(offsetWithSeconds.isValid());
        REQUIRE_EQUAL(offsetWithSeconds.timeOffset(), Duration{Seconds{3723}});

        const auto fraction = DateTime::fromIsoString("2026-05-20T12:30:00.123456789"_el);
        REQUIRE(fraction.isValid());
        REQUIRE_EQUAL(fraction.nanosecondFraction(), Nanoseconds{123456789});

        const auto shortFraction = DateTime::fromIsoString("2026-05-20T12:30:00,123"_el);
        REQUIRE(shortFraction.isValid());
        REQUIRE_EQUAL(shortFraction.nanosecondFraction(), Nanoseconds{123000000});
    }

    void testIsoTimeShiftFormattingOptions() {

        const auto dateTime =
            DateTime{Date::fromYearMonthDay(2026, 5, 20), Time{Hour{12}, Minute{30}, Second{0}}, Seconds{3723}};
        const auto flags = IsoTimeFormatFlags{IsoTimeFormat::Extended} | IsoTimeFormat::TimeShift |
            IsoTimeFormat::TimeShiftUpToSeconds;

        auto timeStdStr = StringConverter{dateTime.toIsoString(flags)}.toStdString();
        REQUIRE_EQUAL(timeStdStr, "2026-05-20 12:30:00+01:02:03");
        auto timeStr = StringConverter{DateTime::epoch(TimeEpoch::Posix)
                                           .toIsoString(
                                               IsoTimeFormatFlags{IsoTimeFormat::Extended} | IsoTimeFormat::TimeShift |
                                               IsoTimeFormat::TimeShiftAlwaysComplete)}
                           .toStdString();
        REQUIRE_EQUAL(timeStr, "1970-01-01 00:00:00+00:00");
    }

    void testLocalMarkerLifecycle() {

        const auto date = Date::fromYearMonthDay(2026, 7, 1);
        const auto time = Time{Hour{12}, Minute{30}};
        const auto localZone = TimeZone::local();
        const auto local = DateTime{date, TimeWithZone{time, localZone}};
        REQUIRE(local.isLocalTime());
        REQUIRE(local.timeZone().isLocalTime());
        REQUIRE_EQUAL(local.toString(), "2026-07-01 12:30:00"_el);

        const auto copy = local;
        REQUIRE(copy.isLocalTime());
        REQUIRE(local.added(Duration{Minutes{15}}).isLocalTime());
        REQUIRE(local.added(CalendarDelta{Days{1}}).isLocalTime());

        const auto utc = local.toUtc();
        REQUIRE_FALSE(utc.isLocalTime());
        REQUIRE(utc.timeZone().isUtc());
        const auto zurich = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        REQUIRE_FALSE(local.toTimeZone(zurich).isLocalTime());
        REQUIRE(utc.toTimeZone(localZone).isLocalTime());

        const auto forcedFlags = IsoTimeFormatFlags{IsoTimeFormat::Extended} | IsoTimeFormat::TimeShift;
        const auto forcedText = local.toIsoString(forcedFlags);
        REQUIRE_NOT_EQUAL(forcedText, "2026-07-01 12:30:00"_el);
    }
};
