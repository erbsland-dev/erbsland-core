// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Moments.hpp"

#include <algorithm>

namespace app::constant_time {

void Moments::add(const double value) noexcept {
    ++_count;
    const auto delta = value - _mean;
    _mean += delta / static_cast<double>(_count);
    _m2 += delta * (value - _mean);
}

auto Moments::variance() const noexcept -> double {
    return _count < 2 ? 0.0 : std::max(0.0, _m2 / static_cast<double>(_count - 1));
}

}
