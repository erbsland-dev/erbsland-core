// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EqualsIntegerConstraint.hpp"

#include "ValidationError.hpp"

#include "../value/Value.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

EqualsIntegerConstraint::EqualsIntegerConstraint(const Integer value) : EqualsConstraint(value) {
}

void EqualsIntegerConstraint::validateInteger(const ValidationContext &context, const Integer value) const {
    if (isNotValid(value, context)) {
        throwValidationError(text::StringFormat{"The value {} {}"_el}.build(comparisonText(), _value));
    }
}

void EqualsIntegerConstraint::validateText(const ValidationContext &context, const text::String &value) const {
    if (isNotValid(static_cast<Integer>(value.characterLength().toSizeT()), context)) {
        throwValidationError(
            text::StringFormat{"The number of characters in this text {} {}"_el}.build(comparisonText(), _value));
    }
}

void EqualsIntegerConstraint::validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const {
    if (isNotValid(static_cast<Integer>(value.length().toSizeT()), context)) {
        throwValidationError(text::StringFormat{"The number of bytes {} {}"_el}.build(comparisonText(), _value));
    }
}

void EqualsIntegerConstraint::validateValueList(const ValidationContext &context) const {
    if (isNotValid(static_cast<Integer>(context.value->asValueList().size()), context)) {
        throwValidationError(
            text::StringFormat{"The number of values in this list {} {}"_el}.build(comparisonText(), _value));
    }
}

void EqualsIntegerConstraint::validateSectionWithNames(const ValidationContext &context) const {
    if (isNotValid(static_cast<Integer>(context.value->size()), context)) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section {} {}"_el}.build(comparisonText(), _value));
    }
}

void EqualsIntegerConstraint::validateSectionWithTexts(const ValidationContext &context) const {
    if (isNotValid(static_cast<Integer>(context.value->size()), context)) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section {} {}"_el}.build(comparisonText(), _value));
    }
}

void EqualsIntegerConstraint::validateSectionList(const ValidationContext &context) const {
    if (isNotValid(static_cast<Integer>(context.value->size()), context)) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section list {} {}"_el}.build(comparisonText(), _value));
    }
}

}
