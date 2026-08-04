// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MultipleFloatConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

#include <cmath>
#include <limits>

namespace erbsland::conf::impl {

using namespace text::literals;

MultipleFloatConstraint::MultipleFloatConstraint(const Float divisor) : MultipleConstraint(divisor) {
}

auto MultipleFloatConstraint::isNotValid(const Float tested) const -> bool {
    const auto d = std::abs(_divisor);
    if (d <= std::numeric_limits<Float>::epsilon()) {
        return !isNegated();
    }
    const auto q = tested / _divisor;
    const auto nearest = std::round(q);
    const bool isMultiple = std::abs(q - nearest) < std::numeric_limits<Float>::epsilon();
    if (isNegated()) {
        return isMultiple;
    }
    return !isMultiple;
}

void MultipleFloatConstraint::validateFloat(
    [[maybe_unused]] const ValidationContext &context, const Float value) const {
    if (isNotValid(value)) {
        throwValidationError(
            text::StringFormat{"The value {} {:.6} (within platform tolerance)"_el}.build(comparisonText(), _divisor));
    }
}

}
