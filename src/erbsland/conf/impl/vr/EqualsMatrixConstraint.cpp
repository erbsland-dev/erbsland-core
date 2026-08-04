// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EqualsMatrixConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

EqualsMatrixConstraint::EqualsMatrixConstraint(const Integer rows, const Integer columns) :
    EqualsConstraint(rows), _columns{columns} {
}

auto EqualsMatrixConstraint::isNotValidColumns(const Integer &validatedValue, const ValidationContext &context) const
    -> bool {
    if (isNegated()) {
        return isEqual(validatedValue, _columns, context);
    }
    return !isEqual(validatedValue, _columns, context);
}

void EqualsMatrixConstraint::validateValueList(const ValidationContext &context) const {
    const auto &value = context.value;
    if (isNotValid(static_cast<Integer>(value->size()), context)) {
        throwValidationError(text::StringFormat{"The number of rows {} {}"_el}.build(comparisonText(), _value));
    }
    for (const auto &columns : *value) {
        if (isNotValidColumns(static_cast<Integer>(columns->size()), context)) {
            throwValidationError(
                text::StringFormat{"The number of columns {} {}"_el}.build(comparisonText(), _columns));
        }
    }
}

}
