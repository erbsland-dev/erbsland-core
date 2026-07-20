// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MultipleConstraint.hpp"

#include "ValidationContext.hpp"
#include "ValidationError.hpp"

#include "../value/Value.hpp"

#include "../../../text/StringFormat.hpp"

#include <cmath>
#include <limits>

namespace erbsland::conf::impl {

using namespace text::literals;

namespace {
[[nodiscard]] constexpr auto absInt(Integer value) -> Integer {
    return value < 0 ? -value : value;
}
}

MultipleIntegerConstraint::MultipleIntegerConstraint(const Integer divisor) : MultipleConstraint(divisor) {
}

auto MultipleIntegerConstraint::isNotValid(const Integer tested) const -> bool {
    const auto d = absInt(_divisor);
    if (d == 0) {
        // cannot be a multiple of zero
        return !isNegated(); // if negated, everything (except 0) is valid; mark invalid only when not negated
    }
    const auto m = tested % d;
    const bool isMultiple = (m == 0);
    if (isNegated()) {
        return isMultiple;
    }
    return !isMultiple;
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

MultipleMatrixConstraint::MultipleMatrixConstraint(const Integer rowsDivisor, const Integer columnsDivisor) :
    MultipleConstraint(rowsDivisor), _columnsDivisor(columnsDivisor) {
}

auto MultipleMatrixConstraint::isNotValidRows(const Integer tested) const -> bool {
    const auto d = absInt(_divisor);
    if (d == 0) {
        return !isNegated();
    }
    const bool isMultiple = (tested % d) == 0;
    if (isNegated()) {
        return isMultiple;
    }
    return !isMultiple;
}

auto MultipleMatrixConstraint::isNotValidColumns(const Integer tested) const -> bool {
    const auto d = absInt(_columnsDivisor);
    if (d == 0) {
        return !isNegated();
    }
    const bool isMultiple = (tested % d) == 0;
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

namespace {
template <typename Message>
void requireType(const conf::ValuePtr &node, const ValueType expected, Message &&msg) {
    if (node->type() != expected) {
        throwValidationError(std::forward<Message>(msg));
    }
}
}

auto handleMultipleConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    switch (rule->type().raw()) {
    case vr::RuleType::Integer:
        requireType(node, ValueType::Integer, "The 'multiple' constraint for an integer rule must be an integer"_el);
        if (node->asInteger() == 0) {
            throwValidationError("The 'multiple' divisor must not be zero"_el);
        }
        return std::make_shared<MultipleIntegerConstraint>(node->asInteger());
    case vr::RuleType::Float:
        requireType(node, ValueType::Float, "The 'multiple' constraint for a float rule must be a float"_el);
        if (std::abs(node->asFloat()) <= std::numeric_limits<Float>::epsilon()) {
            throwValidationError("The 'multiple' divisor must not be zero"_el);
        }
        return std::make_shared<MultipleFloatConstraint>(node->asFloat());
    case vr::RuleType::Text:
        requireType(node, ValueType::Integer, "The 'multiple' constraint for a text rule must be an integer"_el);
        if (node->asInteger() == 0) {
            throwValidationError("The 'multiple' divisor must not be zero"_el);
        }
        return std::make_shared<MultipleIntegerConstraint>(node->asInteger());
    case vr::RuleType::Bytes:
        requireType(node, ValueType::Integer, "The 'multiple' constraint for a bytes rule must be an integer"_el);
        if (node->asInteger() == 0) {
            throwValidationError("The 'multiple' divisor must not be zero"_el);
        }
        return std::make_shared<MultipleIntegerConstraint>(node->asInteger());
    case vr::RuleType::ValueList:
        requireType(node, ValueType::Integer, "The 'multiple' constraint for a value list must be an integer"_el);
        if (node->asInteger() == 0) {
            throwValidationError("The 'multiple' divisor must not be zero"_el);
        }
        return std::make_shared<MultipleIntegerConstraint>(node->asInteger());
    case vr::RuleType::ValueMatrix: {
        const auto intList = node->asList<int>();
        if (intList.size() == 2) {
            if (intList[0] == 0 || intList[1] == 0) {
                throwValidationError("The 'multiple' divisors must not be zero"_el);
            }
            return std::make_shared<MultipleMatrixConstraint>(intList[0], intList[1]);
        }
        throwValidationError("The 'multiple' constraint for a value matrix must be a list with two integer values"_el);
    }
    case vr::RuleType::Section:
    case vr::RuleType::SectionList:
    case vr::RuleType::SectionWithTexts:
        requireType(
            node, ValueType::Integer, "The 'multiple' constraint for a section or section list must be an integer"_el);
        if (node->asInteger() == 0) {
            throwValidationError("The 'multiple' divisor must not be zero"_el);
        }
        return std::make_shared<MultipleIntegerConstraint>(node->asInteger());
    default:
        break;
    }
    throwValidationError(
        text::StringFormat{"The 'multiple' constraint is not supported for '{}' rules"_el}.build(
            rule->type().toText()));
}

}
