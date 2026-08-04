// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MinMaxIntegerConstraint.hpp"

#include "ValidationContext.hpp"
#include "ValidationError.hpp"

#include "../../../math/SaturatingMath.hpp"
#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void MinMaxIntegerConstraint::validateInteger(
    [[maybe_unused]] const ValidationContext &context, const Integer value) const {

    if (isNotValid(value)) {
        throwValidationError(text::StringFormat{"The value must be {} {}"_el}.build(comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateText(
    [[maybe_unused]] const ValidationContext &context, const text::String &value) const {

    if (isNotValid(math::saturatingCast<Integer>(value.characterLength().toSizeT()))) {
        throwValidationError(
            text::StringFormat{"The number of characters in this text must be {} {}"_el}.build(
                comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateBytes(
    [[maybe_unused]] const ValidationContext &context, const mem::ByteBlock &value) const {

    if (isNotValid(math::saturatingCast<Integer>(value.length().toSizeT()))) {
        throwValidationError(
            text::StringFormat{"The number of bytes must be {} {}"_el}.build(comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateValueList(const ValidationContext &context) const {
    std::size_t valueCount = 0;
    if (context.value->type().isList()) {
        valueCount = context.value->size();
    }
    if (isNotValid(math::saturatingCast<Integer>(valueCount))) {
        throwValidationError(
            text::StringFormat{"The number of values in this list must be {} {}"_el}.build(comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateSectionList(const ValidationContext &context) const {
    if (isNotValid(math::saturatingCast<Integer>(context.value->size()))) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section list must be {} {}"_el}.build(
                comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateSectionWithNames(const ValidationContext &context) const {
    if (isNotValid(math::saturatingCast<Integer>(context.value->size()))) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section must be {} {}"_el}.build(
                comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateSectionWithTexts(const ValidationContext &context) const {
    if (isNotValid(math::saturatingCast<Integer>(context.value->size()))) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section must be {} {}"_el}.build(
                comparisonText(), _value));
    }
}

}
