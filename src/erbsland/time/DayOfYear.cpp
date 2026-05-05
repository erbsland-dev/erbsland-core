// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DayOfYear.hpp"

#include "Year.hpp"

namespace erbsland::time {

auto DayOfYear::isLast(const Year year) const noexcept -> bool {
    return *this == last(year);
}

auto DayOfYear::last(const Year year) noexcept -> DayOfYear {
    return year.isLeapYear() ? lastMaximum() : lastMinimum();
}

}
