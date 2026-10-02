// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WelchTest.hpp"

#include <cmath>
#include <limits>

namespace app::constant_time {

void WelchTest::add(const double value, const bool population) noexcept {
    _populations[population].add(value);
}

auto WelchTest::isEligible(const uint64_t minimum) const noexcept -> bool {
    return _populations[0].count() >= minimum && _populations[1].count() >= minimum &&
        (_populations[0].variance() > 0 || _populations[1].variance() > 0);
}

auto WelchTest::statistic() const noexcept -> double {
    if (_populations[0].count() < 2 || _populations[1].count() < 2) {
        return 0;
    }
    const auto difference = _populations[0].mean() - _populations[1].mean();
    const auto denominator = std::sqrt(
        _populations[0].variance() / static_cast<double>(_populations[0].count()) +
        _populations[1].variance() / static_cast<double>(_populations[1].count()));
    if (denominator == 0) {
        return difference == 0 ? 0 : std::copysign(std::numeric_limits<double>::infinity(), difference);
    }
    return difference / denominator;
}

}
