// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DayOfWeek.hpp"

#include "../text/Literals.hpp"

#include <array>

namespace erbsland::time {

using namespace text::literals;

auto DayOfWeek::daysToNext(DayOfWeek dayOfWeek) const noexcept -> Days {
    auto delta = dayOfWeek.toAmount() - toAmount();
    if (delta.isNegative()) {
        delta += Days{7};
    }
    return delta;
}

auto DayOfWeek::daysToPrevious(DayOfWeek dayOfWeek) const noexcept -> Days {
    auto delta = toAmount() - dayOfWeek.toAmount();
    if (delta.isNegative()) {
        delta += Days{7};
    }
    return -delta;
}

auto DayOfWeek::toString(const DayOfWeekFormat format) const -> text::StringView {
    const auto index = toAmount().toValue().toSizeT();
    if (format == DayOfWeekFormat::Short) {
        static const auto names =
            std::array<text::StringView, 7>{"Mon"_el, "Tue"_el, "Wed"_el, "Thu"_el, "Fri"_el, "Sat"_el, "Sun"_el};
        return names[index];
    }
    static const auto names = std::array<text::StringView, 7>{
        "Monday"_el, "Tuesday"_el, "Wednesday"_el, "Thursday"_el, "Friday"_el, "Saturday"_el, "Sunday"_el};
    return names[index];
}

}
