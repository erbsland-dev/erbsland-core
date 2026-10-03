// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeTestsTool.hpp"

#include "impl/CalendarDate.hpp"
#include "impl/CalendarDateTime.hpp"
#include "impl/CalendarTime.hpp"

#include <erbsland/BigInteger.hpp>
#include <erbsland/FastRandom.hpp>
#include <erbsland/Set.hpp>
#include <erbsland/StringJoins.hpp>

#include <array>
#include <limits>

namespace tool::test_data {

using namespace el::text::literals;
using impl::CalendarDate;
using impl::CalendarDateTime;
using impl::CalendarTime;

auto TimeTestsTool::identifier() const -> el::String {
    return "generate_time_tests"_el;
}

auto TimeTestsTool::title() const -> el::String {
    return "Generate date and time fixtures"_el;
}

auto TimeTestsTool::description() const -> el::String {
    return "Generate independent Gregorian calendar and time expectations with local fixed-seed random samples."_el;
}

auto TimeTestsTool::defaultDirectory() const -> el::String {
    return "test/unittest/data/time"_el;
}

void TimeTestsTool::addTable(const el::String &name, const el::String &schema, const el::render::ValueList &rows) {
    _outputs.set(el::join(name, ".txt"_el), renderRows(schema, rows));
}

void TimeTestsTool::execute() {
    validateOutputs(
        el::StringList{
            "date_epoch.txt"_el,
            "date_manipulation.txt"_el,
            "date_days_to.txt"_el,
            "date_comparison.txt"_el,
            "time_midnight.txt"_el,
            "time_comparison.txt"_el,
            "time_amount_comparison.txt"_el,
            "time_amount_conversion.txt"_el,
            "datetime_epoch.txt"_el,
            "datetime_add_duration.txt"_el,
            "datetime_comparison.txt"_el});
    generateDates();
    generateTimes();
    generateAmounts();
    generateDateTimes();
    publish();
}

void TimeTestsTool::generateDates() {
    auto random = el::FastRandom{29839};
    auto dates = el::Set<CalendarDate>{{0, 1, 1}, {0, 12, 31}, {1970, 1, 1}, {9999, 12, 31}};
    while (dates.count() < el::ItemCount{124}) {
        dates.insert(CalendarDate::random(random));
    }
    for (auto index = 0; index < 40; ++index) {
        const auto year = random.getInt32(1, 9999);
        dates.insert({year, 1, 1});
        dates.insert({year - 1, 12, 31});
    }
    auto rows = el::render::ValueList{};
    for (const auto &date : dates) {
        rows.append(
            el::render::Value{
                el::render::ValueList{date.days(), date.fields(), date.dayOfYear(), (date.days() + 5) % 7}});
    }
    addTable("date_epoch"_el, "days year month day day_of_year day_of_week"_el, rows);
    auto manipulationRandom = el::FastRandom{29809};
    rows.clear();
    for (const auto &unit : el::StringList{"days"_el, "months"_el, "years"_el}) {
        for (auto index = 0; index < 48; ++index) {
            const auto base = CalendarDate::random(manipulationRandom);
            auto amount = int64_t{};
            auto expected = CalendarDate{};
            if (unit == "days"_el) {
                amount = manipulationRandom.getInt64(-base.days(), 3652424 - base.days());
                expected = CalendarDate::fromDays(base.days() + amount);
            } else if (unit == "months"_el) {
                const auto monthIndex = base.year * 12 + base.month - 1;
                amount = manipulationRandom.getInt64(-monthIndex, 119999 - monthIndex);
                expected = base.addedMonths(amount);
            } else {
                amount = manipulationRandom.getInt64(-base.year, 9999 - base.year);
                expected = base.addedMonths(amount * 12);
            }
            rows.append(el::render::Value{el::render::ValueList{base.fields(), unit, amount, expected.fields()}});
        }
    }
    addTable(
        "date_manipulation"_el,
        "base_year base_month base_day unit amount expected_year expected_month expected_day"_el,
        rows);
    auto differenceRandom = el::FastRandom{21809};
    rows.clear();
    for (auto index = 0; index < 140; ++index) {
        const auto base = CalendarDate::random(differenceRandom);
        const auto amount = differenceRandom.getInt64(-base.days(), 3652424 - base.days());
        rows.append(
            el::render::Value{
                el::render::ValueList{base.fields(), CalendarDate::fromDays(base.days() + amount).fields(), amount}});
    }
    addTable(
        "date_days_to"_el, "source_year source_month source_day target_year target_month target_day days"_el, rows);
    addTable(
        "date_comparison"_el,
        "valid year month day order"_el,
        el::render::ValueList{
            el::render::ValueList{0, 0, 0, 0, -1},
            el::render::ValueList{1, 0, 1, 1, 0},
            el::render::ValueList{1, 2022, 10, 2, 1},
            el::render::ValueList{1, 9999, 12, 31, 2}});
}

void TimeTestsTool::generateTimes() {
    auto random = el::FastRandom{29839};
    auto times = el::Set<CalendarTime>{{}, {23, 59, 59, 999999999}};
    for (auto index = 0; index < 10; ++index) {
        times.insert(CalendarTime::random(random));
    }
    for (auto index = 0; index < 10; ++index) {
        const auto value = CalendarTime::random(random);
        for (
            const auto &time : std::array{
                CalendarTime{value.hour, value.minute, value.second, 0},
                CalendarTime{value.hour, value.minute, 0, 0},
                CalendarTime{value.hour, value.minute, 0, value.nanosecond},
                CalendarTime{value.hour, 0, 0, 0}}) {
            times.insert(time);
            if (time.nanoseconds() > 0) {
                times.insert(CalendarTime::fromNanoseconds(time.nanoseconds() - 1));
            }
        }
    }
    auto rows = el::render::ValueList{};
    for (const auto &time : times) {
        rows.append(el::render::Value{el::render::ValueList{time.fields(), time.nanoseconds()}});
    }
    addTable("time_midnight"_el, "hour minute second nanosecond nanoseconds_since_midnight"_el, rows);
    rows.clear();
    auto order = 0;
    for (
        const auto &time : std::array{
            CalendarTime{},
            CalendarTime{0, 0, 0, 1},
            CalendarTime{0, 0, 1, 0},
            CalendarTime{0, 1, 0, 0},
            CalendarTime{1, 0, 0, 0},
            CalendarTime{18, 22, 59, 108938182},
            CalendarTime{23, 59, 59, 999999999}}) {
        rows.append(el::render::Value{el::render::ValueList{time.fields(), order++}});
    }
    addTable("time_comparison"_el, "hour minute second nanosecond order"_el, rows);
}

void TimeTestsTool::generateAmounts() {
    constexpr auto cMinimum = std::numeric_limits<int64_t>::min();
    constexpr auto cMaximum = std::numeric_limits<int64_t>::max();
    const auto names = el::StringList{
        "Nanoseconds"_el,
        "Microseconds"_el,
        "Milliseconds"_el,
        "Seconds"_el,
        "Minutes"_el,
        "Hours"_el,
        "Days"_el,
        "Weeks"_el};
    const auto scales =
        el::List<int64_t>{1, 1000, 1000000, 1000000000, 60000000000, 3600000000000, 86400000000000, 604800000000000};
    auto rows = el::render::ValueList{};
    auto amountNames = names;
    amountNames.append("Months"_el);
    amountNames.append("Years"_el);
    for (const auto &name : amountNames) {
        auto order = 0;
        for (const auto value : std::array<int64_t, 5>{cMinimum, -2903234, 0, 16039287, cMaximum}) {
            rows.append(el::render::Value{el::render::ValueList{name, value, order++}});
        }
    }
    const auto fieldNames =
        el::StringList{"Second"_el, "Minute"_el, "Hour"_el, "Day"_el, "DayOfYear"_el, "Month"_el, "Year"_el};
    const auto minima = el::List<int>{0, 0, 0, 1, 1, 1, 0};
    const auto maxima = el::List<int>{59, 59, 23, 31, 366, 12, 9999};
    for (auto index = el::ItemIndex{0}; index < el::ItemIndex::end(fieldNames.count()); ++index) {
        auto order = 0;
        for (const auto value : {minima.get(index), maxima.get(index) / 2, maxima.get(index)}) {
            rows.append(el::render::Value{el::render::ValueList{fieldNames.get(index), value, order++}});
        }
    }
    for (const auto name : {"TimeDelta"_el, "Duration"_el}) {
        auto order = 0;
        for (const auto value : std::array<int64_t, 5>{cMinimum, -123456, 0, 987654, cMaximum}) {
            rows.append(el::render::Value{el::render::ValueList{name, value, order++}});
        }
    }
    addTable("time_amount_comparison"_el, "type value order"_el, rows);
    rows.clear();
    for (auto source = el::ItemIndex{0}; source < el::ItemIndex::end(names.count()); ++source) {
        for (
            const auto value : std::array<int64_t, 11>{
                cMinimum,
                -129219392392193,
                -291291198,
                -793291,
                -378,
                0,
                529,
                10290,
                828926009,
                302000928948093,
                cMaximum}) {
            for (auto target = el::ItemIndex{0}; target < el::ItemIndex::end(names.count()); ++target) {
                auto expected =
                    el::BigInteger{value} * el::BigInteger{scales.get(source)} / el::BigInteger{scales.get(target)};
                if (expected < el::BigInteger{cMinimum}) {
                    expected = el::BigInteger{cMinimum};
                }
                if (expected > el::BigInteger{cMaximum}) {
                    expected = el::BigInteger{cMaximum};
                }
                rows.append(
                    el::render::Value{
                        el::render::ValueList{names.get(source), value, names.get(target), expected.toString()}});
            }
        }
    }
    addTable("time_amount_conversion"_el, "source_type value target_type expected"_el, rows);
}

void TimeTestsTool::generateDateTimes() {
    auto random = el::FastRandom{29839};
    auto values =
        el::Set<CalendarDateTime>{{}, {CalendarDate{1970, 1, 1}, {}}, {{9999, 12, 31}, {23, 59, 59, 999999999}}};
    while (values.count() < el::ItemCount{124}) {
        values.insert({CalendarDate::random(random), CalendarTime::random(random)});
    }
    auto rows = el::render::ValueList{};
    for (const auto &value : values) {
        rows.append(
            el::render::Value{el::render::ValueList{value.fields(), value.time.nanosecond / 1000000, value.seconds()}});
    }
    addTable(
        "datetime_epoch"_el, "year month day hour minute second nanosecond millisecond seconds_since_epoch"_el, rows);
    const auto bases = std::array{
        CalendarDateTime{},
        CalendarDateTime{{1970, 12, 31}, {4, 17, 0, 0}},
        CalendarDateTime{{2000, 2, 29}, {17, 59, 13, 100200300}},
        CalendarDateTime{{7022, 8, 1}, {23, 1, 4, 728349}},
        CalendarDateTime{{9999, 12, 31}, {23, 59, 59, 999999999}}};
    auto deltas = el::Set<int64_t>{
        0,
        -1,
        1,
        -60,
        60,
        -3600,
        3600,
        -86400,
        86400,
        std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max()};
    auto power = int64_t{1};
    for (auto exponent = 1; exponent < 19; ++exponent) {
        power *= 10;
        deltas.insert(power);
        deltas.insert(-power);
    }
    rows.clear();
    for (const auto &base : bases) {
        for (const auto delta : deltas) {
            rows.append(
                el::render::Value{el::render::ValueList{
                    base.fields(),
                    delta,
                    base.addedSeconds(el::BigInteger{delta}).fields(),
                    base.addedSeconds(-el::BigInteger{delta}).fields()}});
        }
    }
    addTable(
        "datetime_add_duration"_el,
        "base_year base_month base_day base_hour base_minute base_second base_nanosecond delta_seconds "
        "add_year add_month add_day add_hour add_minute add_second add_nanosecond "
        "subtract_year subtract_month subtract_day subtract_hour subtract_minute subtract_second subtract_nanosecond"_el,
        rows);
    rows = el::render::ValueList{el::render::Value{el::render::ValueList{0, 0, 0, 0, 0, 0, 0, 0, -1}}};
    auto order = 0;
    for (
        const auto &value : std::array{
            CalendarDateTime{},
            CalendarDateTime{{2022, 10, 1}, {17, 40, 12, 128837237}},
            CalendarDateTime{{2022, 10, 2}, {17, 40, 12, 128837236}},
            CalendarDateTime{{2022, 10, 2}, {17, 40, 12, 128837237}},
            CalendarDateTime{{9999, 12, 31}, {23, 59, 59, 999999999}}}) {
        rows.append(el::render::Value{el::render::ValueList{1, value.fields(), order++}});
    }
    addTable("datetime_comparison"_el, "valid year month day hour minute second nanosecond order"_el, rows);
}

}
