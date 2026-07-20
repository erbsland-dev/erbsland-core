// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EqualsConstraint.hpp"

#include "ValidationError.hpp"

#include "../value/Value.hpp"

#include "../../../text/EscapeFormat.hpp"
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

EqualsBooleanConstraint::EqualsBooleanConstraint(const bool value) : EqualsConstraint(value) {
}

void EqualsBooleanConstraint::validateBoolean(const ValidationContext &context, const bool value) const {
    if (isNotValid(value, context)) {
        const auto expectedValue = isNegated() ? !_value : _value;
        throwValidationError(
            text::StringFormat{"The value must be {}"_el}.build(text::String{expectedValue ? "true"_el : "false"_el}));
    }
}

EqualsFloatConstraint::EqualsFloatConstraint(const Float value) : EqualsConstraint(value) {
}

void EqualsFloatConstraint::validateFloat(const ValidationContext &context, const Float value) const {
    if (isNotValid(value, context)) {
        throwValidationError(
            text::StringFormat{"The value {} {:.6} (within platform tolerance)"_el}.build(comparisonText(), _value));
    }
}

void EqualsTextConstraint::validateText(const ValidationContext &context, const text::String &value) const {
    if (isNotValid(value, context)) {
        throwValidationError(
            text::StringFormat{"The text {} \"{:/display}\" ({})"_el}.build(
                comparisonText(), _value, context.rule->caseSensitivity().toString()));
    }
}

void EqualsBytesConstraint::validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const {
    if (isNotValid(value, context)) {
        throwValidationError(
            text::StringFormat{"The byte sequence {} \"{:bytes:maximum=16,truncate=middle}\""_el}.build(
                comparisonText(), _value));
    }
}

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

auto handleEqualsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    auto &node = context.node;
    auto &rule = context.rule;
    switch (rule->type().raw()) {
    case vr::RuleType::Integer:
        if (node->type() != ValueType::Integer) {
            throwValidationError("The 'equals' constraint for an integer rule must be an integer"_el);
        }
        return std::make_shared<EqualsIntegerConstraint>(node->asInteger());
    case vr::RuleType::Float:
        if (node->type() != ValueType::Float) {
            throwValidationError("The 'equals' constraint for a float rule must be a float"_el);
        }
        return std::make_shared<EqualsFloatConstraint>(node->asFloat());
    case vr::RuleType::Text:
        if (node->type() == ValueType::Text) {
            return std::make_shared<EqualsTextConstraint>(node->asText());
        }
        if (node->type() != ValueType::Integer) {
            throwValidationError("The 'equals' constraint for a text rule must be a text or integer"_el);
        }
        return std::make_shared<EqualsIntegerConstraint>(node->asInteger());
    case vr::RuleType::Bytes:
        if (node->type() == ValueType::Bytes) {
            return std::make_shared<EqualsBytesConstraint>(node->asBytes());
        }
        if (node->type() != ValueType::Integer) {
            throwValidationError("The 'equals' constraint for a bytes rule must be a byte sequence or integer"_el);
        }
        return std::make_shared<EqualsIntegerConstraint>(node->asInteger());
    case vr::RuleType::Boolean:
        if (node->type() != ValueType::Boolean) {
            throwValidationError("The 'equals' constraint for a boolean rule must be a boolean"_el);
        }
        return std::make_shared<EqualsBooleanConstraint>(node->asBoolean());
    case vr::RuleType::ValueList:
        if (node->type() != ValueType::Integer) {
            throwValidationError("The 'equals' constraint for a value list must be an integer"_el);
        }
        return std::make_shared<EqualsIntegerConstraint>(node->asInteger());
    case vr::RuleType::ValueMatrix:
        if (auto intList = node->asList<int>(); intList.size() == 2) {
            return std::make_shared<EqualsMatrixConstraint>(intList[0], intList[1]);
        }
        throwValidationError("The 'equals' constraint for a value matrix must be a list with two integer values"_el);
    case vr::RuleType::Section:
    case vr::RuleType::SectionList:
    case vr::RuleType::SectionWithTexts:
        if (node->type() != ValueType::Integer) {
            throwValidationError("The 'equals' constraint for a section or section list must be an integer"_el);
        }
        return std::make_shared<EqualsIntegerConstraint>(node->asInteger());
    default:
        break;
    };
    throwValidationError(
        text::StringFormat{"The 'equals' constraint is not supported for '{}' rules"_el}.build(rule->type().toText()));
}

}
