// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MultipleMatrixConstraint.hpp"

#include "ValidationContext.hpp"
#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

MultipleMatrixConstraint::MultipleMatrixConstraint(const Integer rowsDivisor, const Integer columnsDivisor) :
    MultipleConstraint(rowsDivisor), _columnsDivisor(columnsDivisor) {
}

auto MultipleMatrixConstraint::isNotValidRows(const Integer tested) const -> bool {
    if (_divisor == 0) {
        return !isNegated();
    }
    const bool isMultiple = (tested % _divisor) == 0;
    if (isNegated()) {
        return isMultiple;
    }
    return !isMultiple;
}

auto MultipleMatrixConstraint::isNotValidColumns(const Integer tested) const -> bool {
    if (_columnsDivisor == 0) {
        return !isNegated();
    }
    const bool isMultiple = (tested % _columnsDivisor) == 0;
    if (isNegated()) {
        return isMultiple;
    }
    return !isMultiple;
}

void MultipleMatrixConstraint::validateValueList(const ValidationContext &context) const {
    const auto &value = context.value;
    if (isNotValidRows(static_cast<Integer>(value->size()))) {
        throwValidationError(text::StringFormat{"The number of rows {} {}"_el}.build(comparisonText(), _divisor));
    }
    for (const auto &columns : *value) {
        if (isNotValidColumns(static_cast<Integer>(columns->size()))) {
            throwValidationError(
                text::StringFormat{"The number of columns {} {}"_el}.build(comparisonText(), _columnsDivisor));
        }
    }
}

}
