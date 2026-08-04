// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EqualsConstraint.hpp"

#include "EqualsBooleanConstraint.hpp"
#include "EqualsBytesConstraint.hpp"
#include "EqualsFloatConstraint.hpp"
#include "EqualsIntegerConstraint.hpp"
#include "EqualsMatrixConstraint.hpp"
#include "EqualsTextConstraint.hpp"
#include "ValidationError.hpp"

#include "../value/Value.hpp"

#include "../../../text/EscapeFormat.hpp"
#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

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
