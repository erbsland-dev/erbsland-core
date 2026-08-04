// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MultipleConstraint.hpp"

#include "MultipleFloatConstraint.hpp"
#include "MultipleIntegerConstraint.hpp"
#include "MultipleMatrixConstraint.hpp"
#include "ValidationContext.hpp"
#include "ValidationError.hpp"

#include "../value/Value.hpp"

#include "../../../math/IntegerMath.hpp"
#include "../../../text/StringFormat.hpp"

#include <cmath>
#include <limits>

namespace erbsland::conf::impl {

using namespace text::literals;

auto handleMultipleConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    const auto requireType = [&node](const ValueType expected, const text::StringLiteral &message) -> void {
        if (node->type() != expected) {
            throwValidationError(message);
        }
    };
    switch (rule->type().raw()) {
    case vr::RuleType::Integer:
        requireType(ValueType::Integer, "The 'multiple' constraint for an integer rule must be an integer"_el);
        if (node->asInteger() == 0) {
            throwValidationError("The 'multiple' divisor must not be zero"_el);
        }
        return std::make_shared<MultipleIntegerConstraint>(node->asInteger());
    case vr::RuleType::Float:
        requireType(ValueType::Float, "The 'multiple' constraint for a float rule must be a float"_el);
        if (std::abs(node->asFloat()) <= std::numeric_limits<Float>::epsilon()) {
            throwValidationError("The 'multiple' divisor must not be zero"_el);
        }
        return std::make_shared<MultipleFloatConstraint>(node->asFloat());
    case vr::RuleType::Text:
        requireType(ValueType::Integer, "The 'multiple' constraint for a text rule must be an integer"_el);
        if (node->asInteger() == 0) {
            throwValidationError("The 'multiple' divisor must not be zero"_el);
        }
        return std::make_shared<MultipleIntegerConstraint>(node->asInteger());
    case vr::RuleType::Bytes:
        requireType(ValueType::Integer, "The 'multiple' constraint for a bytes rule must be an integer"_el);
        if (node->asInteger() == 0) {
            throwValidationError("The 'multiple' divisor must not be zero"_el);
        }
        return std::make_shared<MultipleIntegerConstraint>(node->asInteger());
    case vr::RuleType::ValueList:
        requireType(ValueType::Integer, "The 'multiple' constraint for a value list must be an integer"_el);
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
            ValueType::Integer, "The 'multiple' constraint for a section or section list must be an integer"_el);
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
