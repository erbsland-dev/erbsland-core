// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeZoneModel.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/Date.hpp>
#include <erbsland/StringFormat.hpp>

#include <algorithm>

namespace tool::time_zone {

using namespace el::text::literals;

auto TimeZoneModel::monthNumber(const el::String &month) -> int {
    static const auto cMonths = std::array<el::String, 12>{
        "Jan"_el,
        "Feb"_el,
        "Mar"_el,
        "Apr"_el,
        "May"_el,
        "Jun"_el,
        "Jul"_el,
        "Aug"_el,
        "Sep"_el,
        "Oct"_el,
        "Nov"_el,
        "Dec"_el};
    const auto it = std::find(cMonths.begin(), cMonths.end(), month);
    if (it == cMonths.end()) {
        throw el::ApplicationError{"Invalid tzdb Month"_el, month};
    }
    return static_cast<int>(it - cMonths.begin()) + 1;
}

auto TimeZoneModel::weekdayNumber(const el::String &day) -> int {
    static const auto cDays =
        std::array<el::String, 7>{"Mon"_el, "Tue"_el, "Wed"_el, "Thu"_el, "Fri"_el, "Sat"_el, "Sun"_el};
    const auto it = std::find(cDays.begin(), cDays.end(), day);
    if (it == cDays.end()) {
        throw el::ApplicationError{"Invalid tzdb Weekday"_el, day};
    }
    return static_cast<int>(it - cDays.begin());
}

auto TimeZoneModel::resolveDay(const int year, const int month, const el::String &expression) -> int {
    if (expression.isEmpty()) {
        return 1;
    }
    if (expression.startsWith("last"_el)) {
        const auto weekday = el::time::DayOfWeek{static_cast<int8_t>(
            weekdayNumber(expression.slice(el::StringSide::Back, expression.characterLength() - el::CpCount{4})))};
        auto date = el::time::Date::lastDay(el::time::Year{year}, el::time::Month{month});
        while (date.dayOfWeek() != weekday) {
            date = date.previous();
        }
        return date.day().toValue().toRawValue();
    }
    if (const auto parts = expression.splitAtFirst(">="_el)) {
        const auto weekday = el::time::DayOfWeek{static_cast<int8_t>(weekdayNumber(parts->first))};
        auto date = el::time::Date{
            el::time::Year{year}, el::time::Month{month}, el::time::Day{parts->second.toIntegerOrThrow<int>()}};
        while (date.isValid() && date.dayOfWeek() != weekday) {
            date = date.next();
        }
        if (!date.isValid() || date.month().toValue().toRawValue() != month) {
            throw el::ApplicationError{"Invalid tzdb Day Rule"_el, expression};
        }
        return date.day().toValue().toRawValue();
    }
    if (const auto parts = expression.splitAtFirst("<="_el)) {
        const auto weekday = el::time::DayOfWeek{static_cast<int8_t>(weekdayNumber(parts->first))};
        auto date = el::time::Date{
            el::time::Year{year}, el::time::Month{month}, el::time::Day{parts->second.toIntegerOrThrow<int>()}};
        while (date.isValid() && date.dayOfWeek() != weekday) {
            date = date.previous();
        }
        if (!date.isValid() || date.month().toValue().toRawValue() != month) {
            throw el::ApplicationError{"Invalid tzdb Day Rule"_el, expression};
        }
        return date.day().toValue().toRawValue();
    }
    return expression.toIntegerOrThrow<int>();
}

auto TimeZoneModel::civilSeconds(const int year, const int month, const int day, const int seconds) -> int64_t {
    const auto date = el::time::Date{el::time::Year{year}, el::time::Month{month}, el::time::Day{day}};
    if (!date.isValid()) {
        throw el::ApplicationError{
            "Invalid tzdb Civil Date"_el, el::StringFormat{"{}-{}-{}"_el}.build(year, month, day)};
    }
    return static_cast<int64_t>(date.toDaysSinceEpoch().toRawValue()) * 86400LL + seconds;
}

auto TimeZoneModel::lastYear(const Rule &rule) -> int {
    if (rule.yearTo == "only"_el) {
        return rule.yearFrom;
    }
    if (rule.yearTo == "max"_el) {
        return 10000;
    }
    return rule.yearTo.toIntegerOrThrow<int>();
}

auto TimeZoneModel::saveMinutes(const Rule &rule) -> int {
    const auto seconds = parseTime(rule.save).seconds;
    if (seconds % 60 != 0) {
        throw el::ApplicationError{"Invalid tzdb Rule"_el, "SAVE must be a whole number of minutes."_el};
    }
    return seconds / 60;
}

auto TimeZoneModel::ruleSaveSeconds(const Rule &rule) -> int {
    return saveMinutes(rule) * 60;
}

auto TimeZoneModel::transition(const Rule &rule, const int year) -> Transition {
    const auto at = parseTime(rule.at);
    return Transition{
        civilSeconds(year, monthNumber(rule.month), resolveDay(year, monthNumber(rule.month), rule.day), at.seconds),
        at.reference,
        saveMinutes(rule)};
}

auto TimeZoneModel::lastTransitions(const RuleSet &set, int year) const -> std::vector<Transition> {
    auto result = std::vector<Transition>{};
    auto firstYear = 10000;
    for (const auto &rule : set.rules) {
        firstYear = std::min(firstYear, rule.yearFrom);
    }
    while (year >= firstYear && result.size() < 4U) {
        for (const auto &rule : set.rules) {
            if (year >= rule.yearFrom && year <= lastYear(rule)) {
                result.push_back(transition(rule, year));
            }
        }
        --year;
    }
    std::sort(
        result.begin(), result.end(), [](const Transition &a, const Transition &b) { return a.seconds < b.seconds; });
    return result;
}

auto TimeZoneModel::parseUntil(const el::String &text) -> std::pair<int64_t, uint8_t> {
    const auto parts = fields(text);
    if (parts.isEmpty() || parts.count() > el::ItemCount{4U}) {
        throw el::ApplicationError{"Invalid tzdb UNTIL"_el, text};
    }
    const auto year = parts.getRef(el::ItemIndex{0}).toIntegerOrThrow<int>();
    auto month = 1;
    if (parts.count() >= el::ItemCount{2U}) {
        month = monthNumber(parts.getRef(el::ItemIndex::one()));
    }
    auto day = 1;
    if (parts.count() >= el::ItemCount{3U}) {
        day = resolveDay(year, month, parts.getRef(el::ItemIndex{2}));
    }
    auto at = TimeValue{};
    if (parts.count() >= el::ItemCount{4U}) {
        at = parseTime(parts.getRef(el::ItemIndex{3}));
    } else {
        at = parseTime("0"_el);
    }
    return {civilSeconds(year, month, day, at.seconds), at.reference};
}

auto TimeZoneModel::toUtc(const int64_t seconds, const uint8_t reference, const int standard, const int saving)
    -> int64_t {
    if (reference == 0U) {
        return seconds;
    }
    if (reference == 2U) {
        return seconds - standard;
    }
    return seconds - standard - saving;
}

auto TimeZoneModel::untilBoundary(const ZoneEntry &entry) const -> std::pair<uint64_t, int> {
    if (entry.until.isEmpty()) {
        return {0U, 0};
    }
    const auto [localUntil, reference] = parseUntil(entry.until);
    const auto standard = standardOffset(entry);
    if (!hasRuleSet(entry)) {
        const auto saving = fixedRuleOffset(entry);
        const auto utc = toUtc(localUntil, reference, standard, saving);
        if (utc < 0) {
            throw el::ApplicationError{"Invalid tzdb UNTIL"_el, "A boundary precedes the supported epoch."_el};
        }
        return {static_cast<uint64_t>(utc), standard + saving};
    }
    const auto untilFields = fields(entry.until);
    auto points = lastTransitions(*_ruleSets.get(entry.ruleName).value(), untilFields.first().toIntegerOrThrow<int>());
    auto lastSaving = points.size() <= 1U ? 0 : points.front().saveMinutes * 60;
    if (!points.empty()) {
        points.erase(points.begin());
    }
    for (const auto &point : points) {
        const auto utcPoint = toUtc(point.seconds, point.reference, standard, lastSaving);
        if (toUtc(localUntil, reference, standard, lastSaving) >= utcPoint) {
            break;
        }
        lastSaving = point.saveMinutes * 60;
    }
    const auto utc = toUtc(localUntil, reference, standard, lastSaving);
    if (utc < 0) {
        throw el::ApplicationError{"Invalid tzdb UNTIL"_el, "A boundary precedes the supported epoch."_el};
    }
    return {static_cast<uint64_t>(utc), standard + lastSaving};
}

auto TimeZoneModel::encodedRule(const Rule &rule) -> uint64_t {
    const auto yearTo = lastYear(rule);
    if (rule.yearFrom <= 0 || rule.yearFrom > 9999 || yearTo < rule.yearFrom || yearTo > 10000) {
        throw el::ApplicationError{"Invalid tzdb Rule"_el, "Rule year range is unsupported."_el};
    }
    const auto delta = yearTo == 10000 ? 0xFF : yearTo - rule.yearFrom;
    if (delta > 0xFE && delta != 0xFF) {
        throw el::ApplicationError{"Invalid tzdb Rule"_el, "Rule year range is too large."_el};
    }
    auto day = 0;
    auto weekday = 0;
    auto dayRule = 0;
    if (rule.day.startsWith("last"_el)) {
        weekday = weekdayNumber(rule.day.slice(el::StringSide::Back, rule.day.characterLength() - el::CpCount{4})) + 1;
        dayRule = 3;
    } else if (const auto parts = rule.day.splitAtFirst(">="_el)) {
        weekday = weekdayNumber(parts->first) + 1;
        day = parts->second.toIntegerOrThrow<int>();
        dayRule = 1;
    } else if (const auto parts = rule.day.splitAtFirst("<="_el)) {
        weekday = weekdayNumber(parts->first) + 1;
        day = parts->second.toIntegerOrThrow<int>();
        dayRule = 2;
    } else {
        day = rule.day.toIntegerOrThrow<int>();
    }
    const auto at = parseTime(rule.at);
    const auto saving = saveMinutes(rule);
    if (at.seconds % 60 != 0 || day < 0 || day > 31 || saving < -16384 || saving > 16383) {
        throw el::ApplicationError{"Invalid tzdb Rule"_el, "Rule fields exceed the compact representation."_el};
    }
    const auto atMinutes = at.seconds / 60;
    auto value = uint64_t{0};
    value |= static_cast<uint64_t>(rule.yearFrom & 0x3FFF);
    value |= static_cast<uint64_t>(delta & 0xFF) << 14U;
    value |= static_cast<uint64_t>((monthNumber(rule.month) - 1) & 0x0F) << 22U;
    value |= static_cast<uint64_t>(day & 0x1F) << 26U;
    value |= static_cast<uint64_t>(weekday & 0x07) << 31U;
    value |= static_cast<uint64_t>(dayRule & 0x03) << 34U;
    value |= static_cast<uint64_t>(atMinutes & 0x07FF) << 36U;
    value |= static_cast<uint64_t>(at.reference & 0x03) << 47U;
    value |= static_cast<uint64_t>(saving & 0x7FFF) << 49U;
    return value;
}

}
