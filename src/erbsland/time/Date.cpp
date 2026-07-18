// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Date.hpp"

#include "../err/OutOfRangeError.hpp"
#include "../err/OverflowError.hpp"
#include "../text/IntegerFormat.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../unit/CpLength.hpp"

#include <algorithm>

namespace erbsland::time {

using namespace text::literals;

using text::IntegerFormat;
using text::IntegerFormatFlag;
using text::String;

namespace {
constexpr auto cLastValidDay = Days{3652424};
constexpr auto cLastMonthSinceEpoch = Months{9999LL * 12LL + 11LL};
}

Date::Date(const Year year, const Month month, const Day day) noexcept :
    _days{daysFromParts(year, month, day).toValue()} {
}

Date::Date(const Day day, const Month month, const Year year) noexcept : Date{year, month, day} {
}

Date::Date(const Year year, const Day day, const Month month) noexcept : Date{year, month, day} {
}

auto Date::isFirst() const noexcept -> bool {
    return _days == 0;
}

auto Date::isLast() const noexcept -> bool {
    return _days == cLastValidDay.toValue();
}

auto Date::wouldAddSaturate(Days days) const -> bool {
    if (!isValid()) {
        return false;
    }
    days += Days{_days};
    // As `Days` is saturating, unbounded and signed, we can check the bounds.
    return days < Days::zero() || days > cLastValidDay;
}

auto Date::wouldAddSaturate(Months months) const -> bool {
    if (!isValid()) {
        return false;
    }
    const auto dateParts = parts();
    auto monthsSinceEpoch = dateParts.month.toAmount() + Months{dateParts.year.toValue() * 12LL};
    monthsSinceEpoch += months;
    return monthsSinceEpoch < Months::zero() || monthsSinceEpoch > cLastMonthSinceEpoch;
}

auto Date::wouldAddSaturate(Years years) const -> bool {
    return wouldAddSaturate(Months{years.toValue().multiplied(12LL)});
}

auto Date::year() const noexcept -> Year {
    return parts().year;
}

auto Date::month() const noexcept -> Month {
    return parts().month;
}

auto Date::day() const noexcept -> Day {
    return parts().day;
}

auto Date::dayOfYear() const noexcept -> DayOfYear {
    if (!isValid()) {
        return DayOfYear{1};
    }
    const auto [year, month, day] = parts();
    return DayOfYear{(year.daysBeforeMonth(month) + day.toAmount()).toValue() + 1};
}

auto Date::dayOfWeek() const noexcept -> DayOfWeek {
    const auto days = isValid() ? _days : math::SatInt32::zero();
    return DayOfWeek{(days + 5) % 7};
}

auto Date::parts() const noexcept -> DateParts {
    if (!isValid()) {
        return {.year = Year{0}, .month = Month{1}, .day = Day{1}};
    }
    const auto yearParts = Year::extractFromEpoch(Days{_days});
    const auto monthParts = Month::extractMonthAndDay(yearParts.year, yearParts.dayOfYear);
    return {.year = yearParts.year, .month = monthParts.month, .day = monthParts.day};
}

auto Date::added(const Days amount) const noexcept -> Date {
    if (!isValid()) {
        return {};
    }
    const auto days = Days{_days} + amount;
    if (days < Days::zero()) {
        return first();
    }
    if (days > cLastValidDay) {
        return last();
    }
    return Date{days.toValue().cast<Storage>(), PrivateTag{}};
}

auto Date::addedOrThrow(const Days amount) const -> Date {
    if (wouldAddSaturate(amount)) {
        throw err::OverflowError{"Date day addition would exceed supported date range"};
    }
    return added(amount);
}

auto Date::added(Months amount) const noexcept -> Date {
    if (!isValid()) {
        return {};
    }
    const auto [year, month, day] = parts();
    auto monthsSinceEpoch = month.toAmount() + Months{year.toValue() * 12LL};
    monthsSinceEpoch += amount;
    if (monthsSinceEpoch < Months::zero()) {
        return first();
    }
    if (monthsSinceEpoch > cLastMonthSinceEpoch) {
        return last();
    }
    const auto newYear = Year{monthsSinceEpoch.toValue() / 12LL};
    const auto newMonth = Month::fromAmount(monthsSinceEpoch % 12LL);
    const auto daysSinceEpoch = daysFromParts(newYear, newMonth, day.clamped(newYear, newMonth));
    return Date{daysSinceEpoch.toValue().cast<Storage>(), PrivateTag{}};
}

auto Date::addedOrThrow(const Months amount) const -> Date {
    if (wouldAddSaturate(amount)) {
        throw err::OverflowError{"Date month addition would exceed supported date range"};
    }
    return added(amount);
}

auto Date::added(Years amount) const noexcept -> Date {
    return added(Months{amount.toValue().multiplied(int64_t{12})});
}

auto Date::addedOrThrow(const Years amount) const -> Date {
    if (wouldAddSaturate(amount)) {
        throw err::OverflowError{"Date year addition would exceed supported date range"};
    }
    return added(amount);
}

void Date::add(Days amount) noexcept {
    *this = added(amount);
}

void Date::addOrThrow(Days amount) {
    *this = addedOrThrow(amount);
}

void Date::add(Months amount) noexcept {
    *this = added(amount);
}

void Date::addOrThrow(Months amount) {
    *this = addedOrThrow(amount);
}

void Date::add(Years amount) noexcept {
    *this = added(amount);
}

void Date::addOrThrow(Years amount) {
    *this = addedOrThrow(amount);
}

auto Date::next() const noexcept -> Date {
    return isValid() && !isLast() ? Date{_days + 1, PrivateTag{}} : Date{};
}

auto Date::previous() const noexcept -> Date {
    return isValid() && !isFirst() ? Date{_days - 1, PrivateTag{}} : Date{};
}

auto Date::next(DayOfWeek dayOfWeek) const noexcept -> Date {
    if (!isValid()) {
        return {};
    }
    auto delta = this->dayOfWeek().daysToNext(dayOfWeek);
    if (delta.isZero()) {
        delta = Days{7};
    }
    return added(delta);
}

auto Date::previous(DayOfWeek dayOfWeek) const noexcept -> Date {
    if (!isValid()) {
        return {};
    }
    auto delta = this->dayOfWeek().daysToPrevious(dayOfWeek);
    if (delta.isZero()) {
        delta = Days{-7};
    }
    return added(delta);
}

auto Date::daysTo(Date other) const noexcept -> Days {
    return isValid() && other.isValid() ? Days{other._days - _days} : Days{};
}

auto Date::toIsoString(const IsoTimeFormatFlags flags, const DateTimePrecision precision) const -> String {
    if (!isValid()) {
        return {};
    }
    const auto [year, month, day] = parts();
    const auto extended = flags.isSet(IsoTimeFormat::Extended);
    auto yearFormat = IntegerFormat::decimal();
    yearFormat.addFlags(IntegerFormatFlag::ZeroFill).setFieldWidth(unit::CpLength{4U});
    auto partFormat = IntegerFormat::decimal();
    partFormat.addFlags(IntegerFormatFlag::ZeroFill).setFieldWidth(unit::CpLength{2U});
    const auto separator = extended ? String{"-"_el} : String{};
    const auto monthText =
        precision >= DateTimePrecision::Month ? String::fromInteger(month.toValue(), partFormat) : String{};
    const auto dayText =
        precision >= DateTimePrecision::Day ? String::fromInteger(day.toValue(), partFormat) : String{};
    return String::fromJoined(
        {String::fromInteger(year.toValue(), yearFormat),
            precision >= DateTimePrecision::Month ? separator : String{},
            monthText,
            precision >= DateTimePrecision::Day ? separator : String{},
            dayText});
}

auto Date::fromYearMonthDay(const int year, const int month, const int day) noexcept -> Date {
    if (!containsParts(year, month, day)) {
        return {};
    }
    return Date{Year{static_cast<int16_t>(year)}, Month{static_cast<int8_t>(month)}, Day{static_cast<int8_t>(day)}};
}

auto Date::fromYearMonthDayOrThrow(const int year, const int month, const int day) -> Date {
    auto result = fromYearMonthDay(year, month, day);
    if (!result.isValid()) {
        throw err::OutOfRangeError{"Date parts are outside the supported date range"};
    }
    return result;
}

auto Date::fromParts(const Year year, const Month month, const Day day) noexcept -> Date {
    return Date{year, month, day};
}

auto Date::fromPartsOrThrow(const Year year, const Month month, const Day day) -> Date {
    auto result = fromParts(year, month, day);
    if (!result.isValid()) {
        throw err::OutOfRangeError{"Date parts are outside the supported date range"};
    }
    return result;
}

auto Date::fromDaysSinceEpoch(const Days days) noexcept -> Date {
    return days >= Days::zero() && days <= cLastValidDay ? Date{days.toValue().cast<Storage>(), PrivateTag{}} : Date{};
}

auto Date::exists(const Year year, const Month month, const Day day) noexcept -> bool {
    return Date{year, month, day}.isValid();
}

auto Date::firstDay(const Year year) noexcept -> Date {
    return Date{year, Month{1}, Day{1}};
}

auto Date::firstDay(const Year year, const Month month) noexcept -> Date {
    return Date{year, month, Day{1}};
}

auto Date::lastDay(Year year) noexcept -> Date {
    return Date{year, Month{12}, Day{31}};
}

auto Date::lastDay(const Year year, const Month month) noexcept -> Date {
    return Date{year, month, Day::last(year, month)};
}

auto Date::epoch() noexcept -> Date {
    return Date{math::SatInt32{0}, PrivateTag{}};
}

auto Date::first() noexcept -> Date {
    return epoch();
}

auto Date::last() noexcept -> Date {
    return Date{cLastValidDay.toValue().cast<Storage>(), PrivateTag{}};
}

auto Date::containsParts(const int year, const int month, const int day) noexcept -> bool {
    if (year < 0 || year > 9999) {
        return false;
    }
    if (month < 1 || month > 12) {
        return false;
    }
    if (day < 1 || Days{day} > Month{month}.dayCount(Year{year})) {
        return false;
    }
    return true;
}

auto Date::daysFromParts(const Year year, const Month month, const Day day) noexcept -> Days {
    if (day.toAmount() >= month.dayCount(year)) {
        return Days{-1};
    }
    return year.daysSinceEpoch() + year.daysBeforeMonth(month) + day.toAmount();
}
}
