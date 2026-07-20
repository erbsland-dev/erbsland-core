// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MinMaxConstraint.hpp"

#include "ValidationContext.hpp"
#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

#include <cmath>
#include <limits>

namespace erbsland::conf::impl {

using namespace text::literals;

namespace {
[[nodiscard]] auto toInteger(const std::size_t value) noexcept -> Integer {
    if (value > static_cast<std::size_t>(std::numeric_limits<Integer>::max())) {
        return std::numeric_limits<Integer>::max();
    }
    return static_cast<Integer>(value);
}
}

void MinMaxIntegerConstraint::validateInteger(
    [[maybe_unused]] const ValidationContext &context, const Integer value) const {

    if (isNotValid(value)) {
        throwValidationError(text::StringFormat{"The value must be {} {}"_el}.build(comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateText(
    [[maybe_unused]] const ValidationContext &context, const text::String &value) const {

    if (isNotValid(toInteger(value.characterLength().toSizeT()))) {
        throwValidationError(
            text::StringFormat{"The number of characters in this text must be {} {}"_el}.build(
                comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateBytes(
    [[maybe_unused]] const ValidationContext &context, const mem::ByteBlock &value) const {

    if (isNotValid(toInteger(value.length().toSizeT()))) {
        throwValidationError(
            text::StringFormat{"The number of bytes must be {} {}"_el}.build(comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateValueList(const ValidationContext &context) const {
    std::size_t valueCount = 0;
    if (context.value->type().isList()) {
        valueCount = context.value->size();
    }
    if (isNotValid(toInteger(valueCount))) {
        throwValidationError(
            text::StringFormat{"The number of values in this list must be {} {}"_el}.build(comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateSectionList(const ValidationContext &context) const {
    if (isNotValid(toInteger(context.value->size()))) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section list must be {} {}"_el}.build(
                comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateSectionWithNames(const ValidationContext &context) const {
    if (isNotValid(toInteger(context.value->size()))) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section must be {} {}"_el}.build(
                comparisonText(), _value));
    }
}

void MinMaxIntegerConstraint::validateSectionWithTexts(const ValidationContext &context) const {
    if (isNotValid(toInteger(context.value->size()))) {
        throwValidationError(
            text::StringFormat{"The number of entries in this section must be {} {}"_el}.build(
                comparisonText(), _value));
    }
}

void MinMaxFloatConstraint::validateFloat([[maybe_unused]] const ValidationContext &context, const Float value) const {

    if (std::isnan(value) || isNotValid(value)) {
        throwValidationError(text::StringFormat{"The value must be {} {}"_el}.build(comparisonText(), _value));
    }
}

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
    if (isNotValid(toInteger(rowCount))) {
        throwValidationError(
            text::StringFormat{"The number of rows in this value matrix must be {} {}"_el}.build(
                comparisonText(), _value));
    }
    for (const auto &columns : *value) {
        std::size_t columnCount = 1;
        if (columns->type().isList()) {
            columnCount = columns->size();
        }
        if (isSecondNotValid(toInteger(columnCount))) {
            throwValidationError(
                text::StringFormat{"The number of columns in this row must be {} {}"_el}.build(
                    comparisonText(), _second),
                value->namePath(),
                value->location());
        }
    }
}

void MinMaxDateConstraint::validateDate(
    [[maybe_unused]] const ValidationContext &context, const time::Date &value) const {

    if (isNotValid(value)) {
        throwValidationError(
            text::StringFormat{"The date must be {} {}"_el}.build(comparisonText(), _value.toString()));
    }
}

void MinMaxDateConstraint::validateDateTime(
    [[maybe_unused]] const ValidationContext &context, const time::DateTime &value) const {

    if (isNotValid(value.date())) {
        throwValidationError(
            text::StringFormat{"The date in this date-time must be {} {}"_el}.build(
                comparisonText(), _value.toString()));
    }
}

void MinMaxDateTimeConstraint::validateDate(
    [[maybe_unused]] const ValidationContext &context, const time::Date &value) const {

    if (isNotValid(time::DateTime{value, time::Time{}})) {
        throwValidationError(
            text::StringFormat{"The date must be {} {}"_el}.build(comparisonText(), _value.date().toString()));
    }
}

void MinMaxDateTimeConstraint::validateDateTime(
    [[maybe_unused]] const ValidationContext &context, const time::DateTime &value) const {

    if (isNotValid(value)) {
        throwValidationError(
            text::StringFormat{"The date-time must be {} {}"_el}.build(comparisonText(), _value.toString()));
    }
}

namespace {
template <typename tConstraint, typename tValueType>
[[nodiscard]] auto createConstraint(
    const MinMaxConstraint::MinOrMax minOrMax, const RulePtr &rule, const conf::ValuePtr &node) -> ConstraintPtr {

    if (node->type() != ValueType::from<tValueType>()) {
        throwValidationError(
            text::StringFormat{"The '{}' constraint for the '{}' rule must be of the type {}"_el}.build(
                node->name().asText(), rule->type().toText(), ValueType::from<tValueType>().toText()));
    }
    return std::make_shared<tConstraint>(minOrMax, node->asType<tValueType>());
}
}

auto handleMinMaxConstraint(const MinMaxConstraint::MinOrMax minOrMax, const ConstraintHandlerContext &context)
    -> ConstraintPtr {

    const auto &node = context.node;
    const auto &rule = context.rule;
    switch (rule->type().raw()) {
    case vr::RuleType::Integer:
        return createConstraint<MinMaxIntegerConstraint, Integer>(minOrMax, rule, node);
    case vr::RuleType::Float:
        return createConstraint<MinMaxFloatConstraint, Float>(minOrMax, rule, node);
    case vr::RuleType::Text:
    case vr::RuleType::Bytes:
        return createConstraint<MinMaxIntegerConstraint, Integer>(minOrMax, rule, node);
    case vr::RuleType::Date:
        return createConstraint<MinMaxDateConstraint, time::Date>(minOrMax, rule, node);
    case vr::RuleType::DateTime:
        return createConstraint<MinMaxDateTimeConstraint, time::DateTime>(minOrMax, rule, node);
    case vr::RuleType::ValueList:
        return createConstraint<MinMaxIntegerConstraint, Integer>(minOrMax, rule, node);
    case vr::RuleType::ValueMatrix:
        if (auto intList = node->asList<int>(); intList.size() == 2) {
            return std::make_shared<MinMaxMatrixConstraint>(minOrMax, intList[0], intList[1]);
        }
        throwValidationError(
            text::StringFormat{"The '{}' constraint for a value matrix must be a list with two integer values"_el}
                .build(node->name().asText()));
    case vr::RuleType::Section:
    case vr::RuleType::SectionList:
    case vr::RuleType::SectionWithTexts:
        return createConstraint<MinMaxIntegerConstraint, Integer>(minOrMax, rule, node);
    default:
        break;
    }
    throwValidationError(
        text::StringFormat{"The '{}' constraint is not supported for '{}' rules"_el}.build(
            node->name().asText(), rule->type().toText()));
}

auto handleMinimumConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    return handleMinMaxConstraint(MinMaxConstraint::Min, context);
}

auto handleMaximumConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    return handleMinMaxConstraint(MinMaxConstraint::Max, context);
}

}
