// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CalendarDate.hpp"

#include <erbsland/StringFormat.hpp>

#include <algorithm>
#include <array>

namespace tool::test_data::impl {

using namespace el::text::literals;

auto CalendarDate::monthDays(const int year, const int month) -> int {
    constexpr auto cMonthDays = std::array{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
        return 29;
    }
    return cMonthDays.at(static_cast<std::size_t>(month - 1));
}

auto CalendarDate::yearStart(const int year) -> int64_t {
    return 365LL * year + (year + 3) / 4 - (year + 99) / 100 + (year + 399) / 400;
}

auto CalendarDate::dayOfYear() const -> int {
    auto result = day;
    for (auto index = 1; index < month; ++index) {
        result += monthDays(year, index);
    }
    return result;
}

auto CalendarDate::days() const -> int64_t {
    return yearStart(year) + dayOfYear() - 1;
}

auto CalendarDate::fromDays(int64_t days) -> CalendarDate {
    days = std::clamp(days, int64_t{0}, int64_t{3652424});
    auto low = 0;
    auto high = 10000;
    while (low + 1 < high) {
        const auto middle = (low + high) / 2;
        if (yearStart(middle) <= days) {
            low = middle;
        } else {
            high = middle;
        }
    }
    auto remaining = static_cast<int>(days - yearStart(low));
    auto month = 1;
    while (remaining >= monthDays(low, month)) {
        remaining -= monthDays(low, month);
        ++month;
    }
    return {low, month, remaining + 1};
}

auto CalendarDate::addedMonths(const int64_t amount) const -> CalendarDate {
    const auto base = year * 12LL + month - 1;
    if (amount < -base) {
        return {};
    }
    if (amount > 119999 - base) {
        return {9999, 12, 31};
    }
    const auto index = base + amount;
    const auto targetYear = static_cast<int>(index / 12);
    const auto targetMonth = static_cast<int>(index % 12 + 1);
    return {targetYear, targetMonth, std::min(day, monthDays(targetYear, targetMonth))};
}

auto CalendarDate::fields() const -> el::String {
    return el::StringFormat{"{} {} {}"_el}.build(year, month, day);
}

auto CalendarDate::random(el::FastRandom &random) -> CalendarDate {
    while (true) {
        const auto value = CalendarDate{random.getInt32(0, 9999), random.getInt32(1, 12), random.getInt32(1, 31)};
        if (value.day <= monthDays(value.year, value.month)) {
            return value;
        }
    }
}

}
