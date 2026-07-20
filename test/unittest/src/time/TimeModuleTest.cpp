// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/time/all.hpp>
#include <erbsland/time/StdFormatForTime.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::time;

using namespace el::text::literals;

using el::text::StringConverter;
using el::unit::Version;

TESTED_TARGETS(
    time Year Month Day DayOfYear DayOfWeek Hour Minute Second Date Time DateTime Duration TimeDelta TimePoint
        ElapsedTimer TimeZone TimeZoneId TimeOffset Nanoseconds Microseconds Milliseconds Seconds Minutes Hours Days
            Weeks Months Years)
class TimeModuleTest final : public el::UnitTest {
public:
    void testDateEpochAndCalendarConversions() {

        const auto epoch = Date::epoch();
        REQUIRE(epoch.isValid());
        REQUIRE_EQUAL(epoch.toDaysSinceEpoch(), Days{0});
        REQUIRE_EQUAL(epoch.year(), Year{0});
        REQUIRE_EQUAL(epoch.month(), Month{1});
        REQUIRE_EQUAL(epoch.day(), Day{1});
        REQUIRE_EQUAL(epoch.dayOfWeek(), DayOfWeek::saturday());
        REQUIRE_EQUAL(StringConverter{epoch.toIsoString()}.toStdString(), "0000-01-01");

        const auto posixEpoch = Date::fromYearMonthDay(1970, 1, 1);
        REQUIRE_EQUAL(posixEpoch.toDaysSinceEpoch(), Days{719528});
        REQUIRE_EQUAL(posixEpoch.dayOfWeek(), DayOfWeek::thursday());
        REQUIRE_EQUAL(posixEpoch.dayOfYear(), DayOfYear{1});
        REQUIRE_EQUAL(Date::last().toDaysSinceEpoch(), Days{3652424});

        REQUIRE(Date::exists(Year{2000}, Month{2}, Day{29}));
        REQUIRE_FALSE(Date::fromYearMonthDay(1900, 2, 29).isValid());
        REQUIRE_EQUAL(Date::lastDay(Year{2024}, Month{2}), Date::fromYearMonthDay(2024, 2, 29));
        REQUIRE_EQUAL(Date::fromYearMonthDay(2024, 1, 31).added(Months{1}), Date::fromYearMonthDay(2024, 2, 29));
        REQUIRE_EQUAL(Date::fromYearMonthDay(2024, 3, 1).daysTo(Date::fromYearMonthDay(2024, 3, 8)), Days{7});
    }

    void testTimeAndAmounts() {

        REQUIRE_EQUAL(Hours{1}.converted<Seconds>(), Seconds{3600});
        REQUIRE_EQUAL(Weeks{1}.converted<Days>(), Days{7});
        REQUIRE_EQUAL(Nanoseconds{1500000000}.converted<Seconds>(), Seconds{1});

        const auto duration =
            Duration{Duration::Parts{.seconds = Seconds{3}, .minutes = Minutes{2}, .hours = Hours{1}}};
        REQUIRE_EQUAL(duration.toSeconds(), Seconds{3723});
        REQUIRE_EQUAL(duration.hours(), Hours{1});
        REQUIRE_EQUAL(duration.minutes(), Minutes{2});
        REQUIRE_EQUAL(duration.seconds(), Seconds{3});

        const auto time = Time{Hour{23}, Minute{59}, Second{58}, Nanoseconds{123456789}};
        REQUIRE_EQUAL(
            StringConverter{time.toIsoString(cDefaultTimeFormat, DateTimePrecision::Nanosecond)}.toStdString(),
            "23:59:58,123456789");

        auto wrapped = Time{Hour{23}, Minute{59}, Second{59}};
        REQUIRE_EQUAL(wrapped.addWithWrap(TimeDelta{Seconds{2}}), Days{1});
        REQUIRE_EQUAL(wrapped, (Time{Hour{0}, Minute{0}, Second{1}}));
    }

    void testDateTimeIsoParsingAndFormatting() {

        const auto posixEpoch = DateTime::posixEpoch();
        REQUIRE_EQUAL(posixEpoch.toSecondsSinceEpoch(), Days{719528}.converted<Seconds>());
        REQUIRE_EQUAL(posixEpoch.toTimeT(), 0);
        REQUIRE_EQUAL(DateTime::fromTimeT(0), posixEpoch);

        const auto withOffset = DateTime::fromIsoString("1970-01-01T01:00:00+01:00"_el);
        REQUIRE(withOffset.isValid());
        REQUIRE_EQUAL(withOffset.toUtc(), posixEpoch);
        REQUIRE_EQUAL(
            StringConverter{
                (withOffset.toIsoString(IsoTimeFormatFlags{IsoTimeFormat::Extended} | IsoTimeFormat::TimeShift))}
                .toStdString(),
            "1970-01-01 01:00:00+01:00");

        REQUIRE_EQUAL(
            StringConverter{DateTime::fromIsoString("2026-05-20"_el, DateTimePrecision::Day).toIsoString()}
                .toStdString(),
            "2026-05-20 00:00:00");
        REQUIRE_FALSE(DateTime::fromIsoString("2026-05"_el, DateTimePrecision::Day).isValid());
        REQUIRE_FALSE(DateTime::fromIsoString("2026-02-29"_el).isValid());
        REQUIRE_THROWS(DateTime::fromIsoStringOrThrow("not a date"_el));
    }

    void testTimeZones() {

        REQUIRE(TimeZone::utc().isUtc());
        REQUIRE(TimeZone::isValidName("UTC"_el));
        REQUIRE(TimeZone::isValidName("Europe/Zurich"_el));
        REQUIRE_FALSE(TimeZone::isValidName("Mars/Olympus"_el));
        REQUIRE_EQUAL(TimeZone::databaseVersion(), (Version{1, 2026, 2}));

        const auto fixedOffset = TimeZone{Hours{5}, Minutes{30}};
        REQUIRE(fixedOffset.isStaticOffset());
        REQUIRE_EQUAL(fixedOffset.staticOffset(), Duration{Seconds{19800}});

        const auto zurich = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        REQUIRE(zurich.isNamed());
        REQUIRE_EQUAL(StringConverter{zurich.name()}.toStdString(), "Europe/Zurich");

        const auto summerUtc = DateTime{Date::fromYearMonthDay(2026, 7, 1), Time{Hour{12}, Minute{0}}};
        const auto summerLocal = summerUtc.toTimeZone(zurich);
        REQUIRE_EQUAL(summerLocal.hour(), Hour{14});
        REQUIRE_EQUAL(summerLocal.timeOffset(), Duration{Hours{2}});
        REQUIRE_EQUAL(StringConverter{summerLocal.timeZoneAbbreviation()}.toStdString(), "CEST");

        const auto winterUtc = DateTime{Date::fromYearMonthDay(2026, 1, 1), Time{Hour{12}, Minute{0}}};
        const auto winterLocal = winterUtc.toTimeZone(zurich);
        REQUIRE_EQUAL(winterLocal.hour(), Hour{13});
        REQUIRE_EQUAL(winterLocal.timeOffset(), Duration{Hours{1}});
        REQUIRE_EQUAL(StringConverter{winterLocal.timeZoneAbbreviation()}.toStdString(), "CET");

        const auto foldDate = Date::fromYearMonthDay(2026, 10, 25);
        const auto firstFold = DateTime{foldDate, Time{Hour{2}, Minute{30}}, zurich, TimeOccurrenceInFold::First};
        const auto secondFold = DateTime{foldDate, Time{Hour{2}, Minute{30}}, zurich, TimeOccurrenceInFold::Second};
        REQUIRE_EQUAL(firstFold.timeOffset(), Duration{Hours{2}});
        REQUIRE_EQUAL(secondFold.timeOffset(), Duration{Hours{1}});
        REQUIRE_EQUAL(firstFold.durationTo(secondFold), Duration{Hours{1}});
    }
};
