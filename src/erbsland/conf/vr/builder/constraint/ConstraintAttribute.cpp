// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConstraintAttribute.hpp"

#include "../../../../text/StringFormat.hpp"
#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void ConstraintAttribute::requireRuleTypeForConstraint(
    const Rule &rule, const text::String &constraintName, const std::initializer_list<vr::RuleType> supportedTypes) {
    if (!hasRuleType(rule, supportedTypes)) {
        throwUnsupportedConstraint(rule, constraintName);
    }
}

auto ConstraintAttribute::hasRuleType(const Rule &rule, const std::initializer_list<vr::RuleType> supportedTypes)
    -> bool {
    for (const auto supportedType : supportedTypes) {
        if (rule.type() == supportedType) {
            return true;
        }
    }
    return false;
}

void ConstraintAttribute::throwUnsupportedConstraint(const Rule &rule, const text::String &constraintName) {
    throwValidationError(
        text::StringFormat{"The '{}' constraint is not supported for '{}' rules"_el}.build(
            constraintName, rule.type().toText()));
}

}
