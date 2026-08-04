// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MinMaxMatrixConstraint.hpp"

#include "ValidationContext.hpp"
#include "ValidationError.hpp"

#include "../../../math/SaturatingMath.hpp"
#include "../../../text/StringFormat.hpp"

#include <cstddef>

namespace erbsland::conf::impl {

using namespace text::literals;

auto MinMaxMatrixConstraint::isSecondNotValid(const Integer validatedValue) const -> bool {
    if (isNegated()) {
        return !compare(validatedValue, _second);
    }
    return compare(validatedValue, _second);
}

void MinMaxMatrixConstraint::validateValueList(const ValidationContext &context) const {
    const auto &value = context.value;
    std::size_t rowCount = 0;
    if (value->type().isList()) {
        rowCount = value->size();
    }
    if (isNotValid(math::saturatingCast<Integer>(rowCount))) {
        throwValidationError(
            text::StringFormat{"The number of rows in this value matrix must be {} {}"_el}.build(
                comparisonText(), _value));
    }
    for (const auto &columns : *value) {
        std::size_t columnCount = 1;
        if (columns->type().isList()) {
            columnCount = columns->size();
        }
        if (isSecondNotValid(math::saturatingCast<Integer>(columnCount))) {
            throwValidationError(
                text::StringFormat{"The number of columns in this row must be {} {}"_el}.build(
                    comparisonText(), _second),
                value->namePath(),
                value->location());
        }
    }
}

}
