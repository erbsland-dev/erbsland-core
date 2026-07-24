// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Statistics.hpp"

#include <cmath>

namespace erbsland::profiling {

auto Statistics::calculate(List<double> values) -> Statistics {
    using namespace text::literals;
    if (values.isEmpty()) {
        throw ApplicationError{"Cannot calculate statistics for an empty sample list."_el};
    }
    values.sort();
    const auto count = values.count().toSizeT();
    auto result = Statistics{};
    result.minimum = values.toRawValue().front();
    result.maximum = values.toRawValue().back();
    result.median = values.toRawValue()[count / 2U];
    auto total = 0.0;
    for (const auto value : values) {
        total += value;
    }
    result.mean = total / static_cast<double>(count);
    const auto percentileIndex =
        std::min(count - 1U, static_cast<std::size_t>(std::ceil(static_cast<double>(count) * 0.95)) - 1U);
    result.p95 = values.toRawValue()[percentileIndex];
    return result;
}

auto Statistics::normalizedRate(const std::uint64_t value, const TimeDelta elapsed) noexcept -> double {
    const auto nanoseconds = elapsed.toNanoseconds().toRawValue();
    if (nanoseconds <= 0) {
        return 0.0;
    }
    return static_cast<double>(value) * 1'000'000'000.0 / static_cast<double>(nanoseconds);
}

auto Statistics::fairness(const List<double> &rates) noexcept -> double {
    if (rates.isEmpty()) {
        return 1.0;
    }
    auto minimum = rates.toRawValue().front();
    auto maximum = minimum;
    for (const auto rate : rates) {
        minimum = std::min(minimum, rate);
        maximum = std::max(maximum, rate);
    }
    return maximum <= 0.0 ? 1.0 : std::max(0.0, minimum / maximum);
}

}
