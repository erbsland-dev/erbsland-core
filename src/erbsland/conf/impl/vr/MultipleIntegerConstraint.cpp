// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MultipleIntegerConstraint.hpp"

#include "ValidationContext.hpp"
#include "ValidationError.hpp"

#include "../../../math/IntegerMath.hpp"
#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

MultipleIntegerConstraint::MultipleIntegerConstraint(const Integer divisor) : MultipleConstraint(divisor) {
}

auto MultipleIntegerConstraint::isNotValid(const Integer tested) const -> bool {
    if (_divisor == 0) {
        // cannot be a multiple of zero
        return !isNegated(); // if negated, everything (except 0) is valid; mark invalid only when not negated
    }
    const auto valueMagnitude = math::toUnsignedAbsolute(tested);
    const auto divisorMagnitude = math::toUnsignedAbsolute(_divisor);
    const auto isMultiple = valueMagnitude % divisorMagnitude == 0U;
    return isNegated() ? isMultiple : !isMultiple;
}

void MultipleIntegerConstraint::validateInteger(
    [[maybe_unused]] const ValidationContext &context, const Integer value) const {

    if (isNotValid(value)) {
        throwValidationError(text::StringFormat{"The value {} {}"_el}.build(comparisonText(), _divisor));
    }
}

void MultipleIntegerConstraint::validateText(
    [[maybe_unused]] const ValidationContext &context, const text::String &value) const {

    if (isNotValid(static_cast<Integer>(value.characterLength().toSizeT()))) {
        throwValidationError(
            text::StringFormat{"The number of characters in this text {} {}"_el}.build(comparisonText(), _divisor));
    }
}

void MultipleIntegerConstraint::validateBytes(
    [[maybe_unused]] const ValidationContext &context, const mem::ByteBlock &value) const {

    if (isNotValid(static_cast<Integer>(value.length().toSizeT()))) {
        throwValidationError(text::StringFormat{"The number of bytes {} {}"_el}.build(comparisonText(), _divisor));
    }
}

void MultipleIntegerConstraint::validateValueList(const ValidationContext &context) const {
    if (isNotValid(static_cast<Integer>(context.value->asValueList().size()))) {
        throwValidationError(
            text::StringFormat{"The number of values in this list {} {}"_el}.build(comparisonText(), _divisor));
    }
}

void MultipleIntegerConstraint::validateSectionWithNames(const ValidationContext &context) const {
    if (isNotValid(static_cast<Integer>(context.value->size()))) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section {} {}"_el}.build(comparisonText(), _divisor));
    }
}

void MultipleIntegerConstraint::validateSectionWithTexts(const ValidationContext &context) const {
    if (isNotValid(static_cast<Integer>(context.value->size()))) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section {} {}"_el}.build(comparisonText(), _divisor));
    }
}

void MultipleIntegerConstraint::validateSectionList(const ValidationContext &context) const {
    if (isNotValid(static_cast<Integer>(context.value->size()))) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section list {} {}"_el}.build(
                comparisonText(), _divisor));
    }
}

}
