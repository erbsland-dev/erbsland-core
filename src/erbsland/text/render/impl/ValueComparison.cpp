// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueComparison.hpp"

#include "../Value.hpp"

#include <cmath>
#include <cstdint>

namespace erbsland::text::render::impl::value_comparison {

auto compareIntegerFloat(const int64_t left, const double right) -> std::partial_ordering {
    if (std::isnan(right)) {
        return std::partial_ordering::unordered;
    }
    constexpr auto cLowerBound = -9223372036854775808.0;
    constexpr auto cUpperBound = 9223372036854775808.0;
    if (right < cLowerBound) {
        return std::partial_ordering::greater;
    }
    if (right >= cUpperBound) {
        return std::partial_ordering::less;
    }
    const auto truncated = static_cast<int64_t>(right);
    if (left < truncated) {
        return std::partial_ordering::less;
    }
    if (left > truncated) {
        return std::partial_ordering::greater;
    }
    const auto converted = static_cast<double>(truncated);
    if (converted < right) {
        return std::partial_ordering::less;
    }
    if (converted > right) {
        return std::partial_ordering::greater;
    }
    return std::partial_ordering::equivalent;
}

auto reverseOrdering(const std::partial_ordering value) noexcept -> std::partial_ordering {
    if (value < 0) {
        return std::partial_ordering::greater;
    }
    if (value > 0) {
        return std::partial_ordering::less;
    }
    return value;
}

auto compareNumbers(const Value &left, const Value &right) -> std::partial_ordering {
    if (left.isInteger() && right.isInteger()) {
        if (left.asInteger() < right.asInteger()) {
            return std::partial_ordering::less;
        }
        if (left.asInteger() > right.asInteger()) {
            return std::partial_ordering::greater;
        }
        return std::partial_ordering::equivalent;
    }
    if (left.isInteger()) {
        return compareIntegerFloat(left.asInteger(), right.asFloat());
    }
    if (right.isInteger()) {
        return reverseOrdering(compareIntegerFloat(right.asInteger(), left.asFloat()));
    }
    if (std::isnan(left.asFloat()) || std::isnan(right.asFloat())) {
        return std::partial_ordering::unordered;
    }
    if (left.asFloat() < right.asFloat()) {
        return std::partial_ordering::less;
    }
    if (left.asFloat() > right.asFloat()) {
        return std::partial_ordering::greater;
    }
    return std::partial_ordering::equivalent;
}

}
