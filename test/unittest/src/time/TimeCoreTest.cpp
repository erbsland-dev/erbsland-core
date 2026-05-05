// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/SaturatingInteger.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/time/all.hpp>
#include <erbsland/time/tz/TimeOffset.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <concepts>
#include <format>
#include <limits>
#include <string>

using el::math::SatInt64;
using el::text::StringConverter;
using el::unit::Version;
using namespace el::time;

TESTED_TARGETS(
    Year Month Day DayOfYear Hour Minute Second TimePartWithAmount Date DayOfWeek Time Duration TimeDelta DateTime
        TimeZone TimeZoneId TimeOffset TimePoint ElapsedTimer Nanoseconds Microseconds Milliseconds Seconds Minutes
            Hours Days Weeks Months Years SecondsUnitTag MonthsUnitTag YearsUnitTag)
class TimeCoreTest final : public el::UnitTest {
public:
    auto additionalErrorMessages() noexcept -> std::string override {
        return std::format("seconds={} nanoseconds={}", _seconds.toRawValue(), _nanoseconds.toRawValue());
    }

    void testConstructionAndArithmetic() {
        static_assert(std::same_as<typename Seconds::Unit, SecondsUnitTag>);
        static_assert(std::same_as<typename Months::Unit, MonthsUnitTag>);
        static_assert(std::same_as<typename Years::Unit, YearsUnitTag>);
        static_assert(std::same_as<typename Nanoseconds::Value, SatInt64>);
        static_assert(std::same_as<typename Microseconds::Value, SatInt64>);
        static_assert(std::same_as<typename Milliseconds::Value, SatInt64>);
        static_assert(std::same_as<typename Seconds::Value, SatInt64>);
        static_assert(std::same_as<typename Minutes::Value, SatInt64>);
        static_assert(std::same_as<typename Hours::Value, SatInt64>);
        static_assert(std::same_as<typename Days::Value, SatInt64>);
        static_assert(std::same_as<typename Weeks::Value, SatInt64>);
        static_assert(std::same_as<typename Months::Value, SatInt64>);
        static_assert(std::same_as<typename Years::Value, SatInt64>);

        _seconds = Seconds{};
        REQUIRE(_seconds.isZero());
        REQUIRE_FALSE(_seconds.isPositive());
        REQUIRE_FALSE(_seconds.isNegative());
        REQUIRE_EQUAL(_seconds.toRawValue(), 0);

        _seconds = Seconds{5} + Seconds{10};
        REQUIRE_EQUAL(_seconds, Seconds{15});
        _seconds += Seconds{7};
        REQUIRE_EQUAL(_seconds, Seconds{22});
        _seconds -= Seconds{30};
        REQUIRE_EQUAL(_seconds, Seconds{-8});
        REQUIRE(_seconds.isNegative());
        REQUIRE_EQUAL(-_seconds, Seconds{8});
    }

    void testSaturation() {
        constexpr auto maximum = std::numeric_limits<int64_t>::max();
        constexpr auto minimum = std::numeric_limits<int64_t>::min();

        _seconds = Seconds{maximum} + Seconds{1};
        REQUIRE_EQUAL(_seconds.toRawValue(), maximum);
        _seconds = Seconds{minimum} - Seconds{1};
        REQUIRE_EQUAL(_seconds.toRawValue(), minimum);
        _seconds = -Seconds{minimum};
        REQUIRE_EQUAL(_seconds.toRawValue(), maximum);
    }

    void testConversions() {
        REQUIRE_EQUAL(Hours{2}.converted<Seconds>(), Seconds{7200});
        REQUIRE_EQUAL(Weeks{1}.converted<Days>(), Days{7});
        REQUIRE_EQUAL(Nanoseconds{999999999}.converted<Seconds>(), Seconds{});
        REQUIRE_EQUAL(Nanoseconds{-999999999}.converted<Seconds>(), Seconds{});
        _nanoseconds = Milliseconds{6}.converted<Nanoseconds>();
        REQUIRE_EQUAL(_nanoseconds, Nanoseconds{6000000});
        REQUIRE_EQUAL(std::chrono::seconds{Seconds{5}.toRawValue()}, std::chrono::seconds{5});
    }

    void testClampingAndConstants() {
        REQUIRE_EQUAL(Year{-1}, Year{0});
        REQUIRE_EQUAL(Year{10000}, Year{9999});
        REQUIRE_EQUAL(Month{0}, Month{1});
        REQUIRE_EQUAL(Month{20}, Month{12});
        REQUIRE_EQUAL(Hour{80}, Hour{23});
        REQUIRE_EQUAL(Second{-5}, Second{0});
        REQUIRE_EQUAL(Month::january(), Month{1});
        REQUIRE_EQUAL(Month::december(), Month{12});
        REQUIRE(Month::february().hasFixedLength() == false);
        REQUIRE(Month::march().hasFixedLength());
    }

    void testCalendarParts() {
        REQUIRE(Day{29}.exists(Year{2024}, Month{2}));
        REQUIRE_FALSE(Day{29}.exists(Year{2023}, Month{2}));
        REQUIRE_EQUAL(Day::last(Year{2024}, Month{2}), Day{29});
        REQUIRE_EQUAL(Day::last(Year{1900}, Month{2}), Day{28});
        REQUIRE_EQUAL(DayOfYear::last(Year{2000}), DayOfYear{366});
        REQUIRE_EQUAL(DayOfYear::last(Year{1900}), DayOfYear{365});

        auto dateParts = Day{31}.next(Year{2024}, Month{12});
        REQUIRE_EQUAL(dateParts.year, Year{2025});
        REQUIRE_EQUAL(dateParts.month, Month{1});
        REQUIRE_EQUAL(dateParts.day, Day{1});

        dateParts = Day{1}.previous(Year{2024}, Month{3});
        REQUIRE_EQUAL(dateParts.year, Year{2024});
        REQUIRE_EQUAL(dateParts.month, Month{2});
        REQUIRE_EQUAL(dateParts.day, Day{29});
    }
    void testDateCommonHandling() {
        _date = {};
        REQUIRE_FALSE(_date.isValid());
        REQUIRE_EQUAL(_date.year(), Year{0});
        REQUIRE_EQUAL(_date.month(), Month{1});
        REQUIRE_EQUAL(_date.day(), Day{1});
        REQUIRE_EQUAL(_date.toDaysSinceEpoch(), Days{-1});

        _date = Date::fromYearMonthDay(2022, 9, 8);
        REQUIRE(_date.isValid());
        auto parts = _date.parts();
        REQUIRE_EQUAL(parts.year, Year{2022});
        REQUIRE_EQUAL(parts.month, Month{9});
        REQUIRE_EQUAL(parts.day, Day{8});
        REQUIRE_EQUAL(_date.dayOfYear(), DayOfYear{251});
        REQUIRE_EQUAL(_date.dayOfWeek(), DayOfWeek::thursday());
        REQUIRE_EQUAL(StringConverter{_date.toIsoString()}.toStdString(), "2022-09-08");
        REQUIRE_EQUAL(StringConverter{_date.toIsoString(IsoTimeFormatFlags{})}.toStdString(), "20220908");

        REQUIRE(Date::exists(Year{0}, Month{1}, Day{1}));
        REQUIRE_FALSE(Date::exists(Year{2022}, Month{2}, Day{31}));
        REQUIRE_THROWS(Date::fromYearMonthDayOrThrow(2022, 2, 31));
    }

    void testManipulationAndBoundaries() {
        REQUIRE_EQUAL(Date::epoch(), Date::first());
        REQUIRE_EQUAL(Date::epoch().dayOfWeek(), DayOfWeek::saturday());
        REQUIRE_EQUAL(Date::fromYearMonthDay(1970, 1, 1).dayOfWeek(), DayOfWeek::thursday());
        REQUIRE_EQUAL(Date::fromYearMonthDay(2024, 1, 31).added(Months{1}), Date::fromYearMonthDay(2024, 2, 29));
        REQUIRE_EQUAL(Date::fromYearMonthDay(2024, 3, 1).previous(), Date::fromYearMonthDay(2024, 2, 29));
        REQUIRE_EQUAL(Date::fromYearMonthDay(2024, 2, 29).next(), Date::fromYearMonthDay(2024, 3, 1));
        REQUIRE_EQUAL(Date::fromYearMonthDay(2024, 3, 1).daysTo(Date::fromYearMonthDay(2024, 3, 8)), Days{7});
        REQUIRE_FALSE(Date::fromYearMonthDay(2024, 3, 1).wouldAddSaturate(Days{7}));
        REQUIRE_EQUAL(Date::first().added(Days{-1}), Date::first());
        REQUIRE(Date::first().wouldAddSaturate(Days{-1}));
        REQUIRE_THROWS(Date::first().addedOrThrow(Days{-1}));
        REQUIRE_EQUAL(Date::last().added(Days{1}), Date::last());
        REQUIRE(Date::last().wouldAddSaturate(Days{1}));
        REQUIRE_THROWS(Date::last().addedOrThrow(Days{1}));
        REQUIRE(Date::last().wouldAddSaturate(Months{1}));
        REQUIRE_THROWS(Date::last().addedOrThrow(Months{1}));
        REQUIRE(Date::last().wouldAddSaturate(Years{1}));
        REQUIRE_THROWS(Date::last().addedOrThrow(Years{1}));
    }

    void testTimeCommonHandling() {
        _time = {};
        REQUIRE(_time.isZero());
        auto timeParts = _time.parts();
        REQUIRE_EQUAL(timeParts.hour, Hour{0});
        REQUIRE_EQUAL(timeParts.minute, Minute{0});
        REQUIRE_EQUAL(timeParts.second, Second{0});
        REQUIRE_EQUAL(timeParts.nanosecondFraction, Nanoseconds{0});

        _time = Time{Hour{4}, Minute{12}, Second{7}, Nanoseconds{123456789}};
        REQUIRE_FALSE(_time.isZero());
        REQUIRE_EQUAL(_time.hour(), Hour{4});
        REQUIRE_EQUAL(_time.minute(), Minute{12});
        REQUIRE_EQUAL(_time.second(), Second{7});
        REQUIRE_EQUAL(_time.millisecondFraction(), Milliseconds{123});
        REQUIRE_EQUAL(_time.nanosecondFraction(), Nanoseconds{123456789});
        REQUIRE_EQUAL(_time.toNanosecondsSinceMidnight(), Nanoseconds{15127123456789});
        REQUIRE_EQUAL(
            StringConverter{_time.toIsoString(cDefaultTimeFormat, DateTimePrecision::Nanosecond)}.toStdString(),
            "04:12:07,123456789");
    }

    void testAddWithWrap() {
        _time = {};
        auto days = _time.addWithWrap(TimeDelta{Seconds{-1}});
        REQUIRE_EQUAL(days, Days{-1});
        REQUIRE_EQUAL(_time, (Time{Hour{23}, Minute{59}, Second{59}}));

        _time = Time{Hour{4}, Minute{12}, Second{7}, Nanoseconds{123456789}};
        days = _time.addWithWrap(Duration{Hours{20}});
        REQUIRE_EQUAL(days, Days{1});
        REQUIRE_EQUAL(_time, (Time{Hour{0}, Minute{12}, Second{7}, Nanoseconds{123456789}}));

        _time = Time{Hour{1}, Minute{0}};
        days = _time.addWithWrap(TimeDelta{Days{2}});
        REQUIRE_EQUAL(days, Days{2});
        REQUIRE_EQUAL(_time, (Time{Hour{1}, Minute{0}}));

        _time = Time{Hour{1}, Minute{0}};
        days = _time.addWithWrap(TimeDelta{Hours{-26}});
        REQUIRE_EQUAL(days, Days{-2});
        REQUIRE_EQUAL(_time, (Time{Hour{23}, Minute{0}}));

        const auto wrapped = _time.addedWithWrap(TimeDelta{Nanoseconds{3}});
        REQUIRE_EQUAL(wrapped.days, Days{});
        REQUIRE_EQUAL(wrapped.time, (Time{Hour{23}, Minute{0}, Second{0}, Nanoseconds{3}}));

        _time = {};
        days = _time.addWithWrap(Duration{Seconds{std::numeric_limits<int64_t>::max()}});
        REQUIRE_EQUAL(days, Days{106751991167300LL});
        REQUIRE_EQUAL(_time, (Time{Hour{15}, Minute{30}, Second{7}}));

        _time = {};
        days = _time.addWithWrap(Duration{Seconds{std::numeric_limits<int64_t>::min()}});
        REQUIRE_EQUAL(days, Days{-106751991167301LL});
        REQUIRE_EQUAL(_time, (Time{Hour{8}, Minute{29}, Second{52}}));
    }

    void testDurationPartsAndArithmetic() {
        auto duration = Duration{Duration::Parts{.seconds = Seconds{9}, .minutes = Minutes{45}, .hours = Hours{6}}};
        REQUIRE_EQUAL(duration.toSeconds(), Seconds{24309});
        REQUIRE_EQUAL(duration.seconds(), Seconds{9});
        REQUIRE_EQUAL(duration.minutes(), Minutes{45});
        REQUIRE_EQUAL(duration.hours(), Hours{6});
        REQUIRE_EQUAL(duration.days(), Days{});

        duration += Duration{Minutes{1}};
        REQUIRE_EQUAL(duration.toSeconds(), Seconds{24369});
        duration -= Duration{Seconds{69}};
        REQUIRE_EQUAL(duration.toSeconds(), Seconds{24300});
        REQUIRE_EQUAL((-duration).toSeconds(), Seconds{-24300});
        REQUIRE_EQUAL(duration.toTimeDelta().toNanoseconds(), Nanoseconds{24300000000000});
        REQUIRE_EQUAL(duration.toTimeDeltaOrThrow().toNanoseconds(), Nanoseconds{24300000000000});
        REQUIRE_FALSE(duration.wouldConvertToTimeDeltaSaturate());

        auto [days, nanoseconds] = Duration{Seconds{90061}}.toDaysAndNanoseconds();
        REQUIRE_EQUAL(days, Days{1});
        REQUIRE_EQUAL(nanoseconds, Nanoseconds{3661000000000});
        const auto negativeDaySplit = Duration{Seconds{-90061}}.toDaysAndNanoseconds();
        REQUIRE_EQUAL(negativeDaySplit.days, Days{-1});
        REQUIRE_EQUAL(negativeDaySplit.nanoseconds, Nanoseconds{-3661000000000});
        REQUIRE_EQUAL(Duration{Hours{36}}.toDaysWithFractions(), 1.5);
        REQUIRE_EQUAL(Duration{Hours{-12}}.toDaysWithFractions(), -0.5);
        REQUIRE(Duration{Seconds{9223372037LL}}.wouldConvertToTimeDeltaSaturate());
        REQUIRE_THROWS(Duration{Seconds{9223372037LL}}.toTimeDeltaOrThrow());
    }

    void testTimeDeltaConversions() {
        using namespace std::chrono_literals;

        auto delta = TimeDelta{100ms};
        REQUIRE_EQUAL(delta.toNanoseconds(), Nanoseconds{100000000});
        REQUIRE_EQUAL(delta.toSeconds(), Seconds{});
        REQUIRE_EQUAL(delta.toSecondsWithFractions(), 0.1);
        REQUIRE_EQUAL(TimeDelta{Hours{36}}.toDaysWithFractions(), 1.5);
        REQUIRE_EQUAL(delta.toStdNanoseconds(), 100000000ns);

        delta += TimeDelta{50ms};
        REQUIRE_EQUAL(delta.toNanoseconds(), Nanoseconds{150000000});
        delta -= TimeDelta{75ms};
        REQUIRE_EQUAL(delta.toNanoseconds(), Nanoseconds{75000000});
        REQUIRE_EQUAL(TimeDelta{Minutes{-5}}.toSeconds(), Seconds{-300});
        REQUIRE_EQUAL(TimeDelta{Seconds{3}}.toDuration(), Duration{Seconds{3}});
    }
    void testConstructionAndEpoch() {
        _dateTime = {};
        REQUIRE_FALSE(_dateTime.isValid());
        REQUIRE(_dateTime.isUtc());
        REQUIRE_EQUAL(_dateTime.year(), Year{0});
        REQUIRE_EQUAL(_dateTime.time(), Time{});

        _dateTime = DateTime{Date{}, Time{Hour{5}, Minute{7}}};
        REQUIRE_FALSE(_dateTime.isValid());
        REQUIRE_EQUAL(_dateTime.utcTime(), Time{});
        REQUIRE_EQUAL(_dateTime.time(), Time{});

        _dateTime =
            DateTime{Date::fromYearMonthDay(2022, 10, 6), Time{Hour{12}, Minute{7}, Second{5}, Nanoseconds{2981029}}};
        REQUIRE(_dateTime.isValid());
        REQUIRE(_dateTime.isUtc());
        const auto dateTimeParts = _dateTime.parts();
        REQUIRE_EQUAL(dateTimeParts.year, Year{2022});
        REQUIRE_EQUAL(dateTimeParts.month, Month{10});
        REQUIRE_EQUAL(dateTimeParts.day, Day{6});
        REQUIRE_EQUAL(dateTimeParts.hour, Hour{12});
        REQUIRE_EQUAL(dateTimeParts.minute, Minute{7});
        REQUIRE_EQUAL(dateTimeParts.second, Second{5});
        REQUIRE_EQUAL(dateTimeParts.nanosecondFraction, Nanoseconds{2981029});
        REQUIRE_EQUAL(_dateTime.millisecondFraction(), Milliseconds{2});
        REQUIRE_EQUAL(DateTime::epoch(), DateTime::first());
        REQUIRE_EQUAL(
            DateTime::last(),
            (DateTime{
                Date{Year{9999}, Month{12}, Day{31}}, Time{Hour{23}, Minute{59}, Second{59}, Nanoseconds{999999999}}}));
    }

    void testStdCompatibilityAndClamping() {
        const auto posixEpoch = DateTime::posixEpoch();
        REQUIRE_EQUAL(posixEpoch.toTimeT(), 0);
        REQUIRE_EQUAL(DateTime::fromTimeT(0), posixEpoch);
        REQUIRE_EQUAL(posixEpoch.toSecondsSinceEpoch(), Days{719528}.converted<Seconds>());

        REQUIRE_EQUAL(DateTime::first().subtracted(Duration{Seconds{1}}), DateTime::first());
        REQUIRE(DateTime::first().wouldSubtractSaturate(Duration{Seconds{1}}));
        REQUIRE_THROWS(DateTime::first().subtractedOrThrow(Duration{Seconds{1}}));
        REQUIRE_EQUAL(DateTime::last().added(Duration{Seconds{1}}), DateTime::last());
        REQUIRE(DateTime::last().wouldAddSaturate(Duration{Seconds{1}}));
        REQUIRE_THROWS(DateTime::last().addedOrThrow(Duration{Seconds{1}}));
        const auto withFraction = DateTime{Date::last(), Time{Hour{23}, Minute{59}, Second{58}, Nanoseconds{7}}};
        REQUIRE_EQUAL(withFraction.added(Duration{Seconds{1}}).nanosecondFraction(), Nanoseconds{7});
    }

    void testIsoFormatAndParse() {
        using namespace el::text::literals;

        _dateTime =
            DateTime{Date::fromYearMonthDay(2022, 10, 6), Time{Hour{12}, Minute{7}, Second{5}, Nanoseconds{2981029}}};
        REQUIRE_EQUAL(
            StringConverter{_dateTime.toIsoString(IsoTimeFormat::Extended, DateTimePrecision::Nanosecond)}
                .toStdString(),
            "2022-10-06 12:07:05,002981029");
        REQUIRE_EQUAL(
            StringConverter{
                _dateTime.toIsoString(
                    IsoTimeFormatFlags{IsoTimeFormat::Extended} | IsoTimeFormat::TimePrefix, DateTimePrecision::Second)}
                .toStdString(),
            "2022-10-06T12:07:05");

        const auto withOffset = DateTime::fromIsoString("1970-01-01T01:00:00+01:00"_el);
        REQUIRE(withOffset.isValid());
        REQUIRE_EQUAL(withOffset.toUtc(), DateTime::posixEpoch());
        REQUIRE_EQUAL(
            StringConverter{DateTime::fromIsoString("2026-05-20"_el, DateTimePrecision::Day).toIsoString()}
                .toStdString(),
            "2026-05-20 00:00:00");
        REQUIRE_FALSE(DateTime::fromIsoString("2026-02-29"_el).isValid());
        REQUIRE_THROWS(DateTime::fromIsoStringOrThrow("not a date"_el));
    }

    void testTimeZoneBasics() {
        using namespace el::text::literals;

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
        REQUIRE_FALSE(zurich.id().isUtc());
    }

    void testTimeZoneConversion() {
        using namespace el::text::literals;

        const auto zurich = TimeZone::fromNameOrThrow("Europe/Zurich"_el);
        const auto summerUtc = DateTime{Date::fromYearMonthDay(2026, 7, 1), Time{Hour{12}, Minute{0}}};
        const auto summerLocal = summerUtc.toTimeZone(zurich);
        REQUIRE_EQUAL(summerLocal.hour(), Hour{14});
        REQUIRE_EQUAL(summerLocal.timeOffset(), Duration{Hours{2}});
        REQUIRE_EQUAL(StringConverter{summerLocal.timeZoneAbbreviation()}.toStdString(), "CEST");

        const auto foldDate = Date::fromYearMonthDay(2026, 10, 25);
        const auto firstFold = DateTime{foldDate, Time{Hour{2}, Minute{30}}, zurich, TimeOccurrenceInFold::First};
        const auto secondFold = DateTime{foldDate, Time{Hour{2}, Minute{30}}, zurich, TimeOccurrenceInFold::Second};
        REQUIRE_EQUAL(firstFold.timeOffset(), Duration{Hours{2}});
        REQUIRE_EQUAL(secondFold.timeOffset(), Duration{Hours{1}});
        REQUIRE_EQUAL(firstFold.durationTo(secondFold), Duration{Hours{1}});
    }

    void testTimeOffsetStorage() {
        auto offset = tz::TimeOffset{};
        REQUIRE(offset.isUtc());
        REQUIRE_FALSE(offset.isStaticOffset());
        REQUIRE_FALSE(offset.isZone());
        REQUIRE_EQUAL(offset.offset(), Seconds{});
        REQUIRE_FALSE(offset.isDst());
        REQUIRE(offset.zoneId().isUtc());
        REQUIRE_EQUAL(offset.abbreviationId(), 0);

        offset = tz::TimeOffset{Seconds{3600}};
        REQUIRE_FALSE(offset.isUtc());
        REQUIRE(offset.isStaticOffset());
        REQUIRE_FALSE(offset.isZone());
        REQUIRE_EQUAL(offset.offset(), Seconds{3600});

        offset = tz::TimeOffset{Seconds{7200}, true, TimeZoneId{1}, 31};
        REQUIRE_FALSE(offset.isUtc());
        REQUIRE_FALSE(offset.isStaticOffset());
        REQUIRE(offset.isZone());
        REQUIRE(offset.isDst());
        REQUIRE_EQUAL(offset.offset(), Seconds{7200});
        REQUIRE_EQUAL(offset.zoneId(), TimeZoneId{1});
        REQUIRE_EQUAL(offset.abbreviationId(), 31);
    }
    void testTimePointArithmetic() {
        const auto start = TimePoint{};
        const auto delta = TimeDelta{Milliseconds{25}};
        const auto end = start + delta;
        REQUIRE_EQUAL(start.timeDeltaTo(end), delta);
        REQUIRE_EQUAL(end - start, delta);
        REQUIRE_EQUAL(end - delta, start);

        auto mutablePoint = start;
        mutablePoint += delta;
        REQUIRE_EQUAL(mutablePoint, end);
        mutablePoint -= delta;
        REQUIRE_EQUAL(mutablePoint, start);
    }

    void testElapsedTimer() {
        auto timer = ElapsedTimer{};
        REQUIRE_FALSE(timer.elapsed().isNegative());
        timer.restart();
        REQUIRE_FALSE(timer.elapsed().isNegative());
    }

private:
    Seconds _seconds;
    Nanoseconds _nanoseconds;
    Date _date;
    Time _time;
    DateTime _dateTime;
};
