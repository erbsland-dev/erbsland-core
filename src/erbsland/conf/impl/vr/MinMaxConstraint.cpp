// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MinMaxConstraint.hpp"

#include "MinMaxDateConstraint.hpp"
#include "MinMaxDateTimeConstraint.hpp"
#include "MinMaxFloatConstraint.hpp"
#include "MinMaxIntegerConstraint.hpp"
#include "MinMaxMatrixConstraint.hpp"
#include "ValidationContext.hpp"
#include "ValidationError.hpp"

#include "../../../math/SaturatingMath.hpp"
#include "../../../text/StringFormat.hpp"

#include <cmath>
#include <cstddef>

namespace erbsland::conf::impl {

using namespace text::literals;

void MinMaxConstraint::throwValueTypeError(const RulePtr &rule, const conf::ValuePtr &node, const ValueType expected) {
    throwValidationError(
        text::StringFormat{"The '{}' constraint for the '{}' rule must be of the type {}"_el}.build(
            node->name().asText(), rule->type().toText(), expected.toText()));
}

auto MinMaxConstraint::createForRule(const MinOrMax minOrMax, const RulePtr &rule, const conf::ValuePtr &node)
    -> ConstraintPtr {
    switch (rule->type().raw()) {
    case vr::RuleType::Integer:
        if (node->type() != ValueType::Integer) {
            throwValueTypeError(rule, node, ValueType::Integer);
        }
        return std::make_shared<MinMaxIntegerConstraint>(minOrMax, node->asInteger());
    case vr::RuleType::Float:
        if (node->type() != ValueType::Float) {
            throwValueTypeError(rule, node, ValueType::Float);
        }
        return std::make_shared<MinMaxFloatConstraint>(minOrMax, node->asFloat());
    case vr::RuleType::Text:
    case vr::RuleType::Bytes:
    case vr::RuleType::ValueList:
    case vr::RuleType::Section:
    case vr::RuleType::SectionList:
    case vr::RuleType::SectionWithTexts:
        if (node->type() != ValueType::Integer) {
            throwValueTypeError(rule, node, ValueType::Integer);
        }
        return std::make_shared<MinMaxIntegerConstraint>(minOrMax, node->asInteger());
    case vr::RuleType::Date:
        if (node->type() != ValueType::Date) {
            throwValueTypeError(rule, node, ValueType::Date);
        }
        return std::make_shared<MinMaxDateConstraint>(minOrMax, node->asDate());
    case vr::RuleType::DateTime:
        if (node->type() != ValueType::DateTime) {
            throwValueTypeError(rule, node, ValueType::DateTime);
        }
        return std::make_shared<MinMaxDateTimeConstraint>(minOrMax, node->asDateTime());
    case vr::RuleType::ValueMatrix:
        if (auto intList = node->asList<int>(); intList.size() == 2) {
            return std::make_shared<MinMaxMatrixConstraint>(minOrMax, intList[0], intList[1]);
        }
        throwValidationError(
            text::StringFormat{"The '{}' constraint for a value matrix must be a list with two integer values"_el}
                .build(node->name().asText()));
    default:
        break;
    }
    throwValidationError(
        text::StringFormat{"The '{}' constraint is not supported for '{}' rules"_el}.build(
            node->name().asText(), rule->type().toText()));
}

auto handleMinimumConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    return MinMaxConstraint::createForRule(MinMaxConstraint::Min, context.rule, context.node);
}

auto handleMaximumConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    return MinMaxConstraint::createForRule(MinMaxConstraint::Max, context.rule, context.node);
}

}
