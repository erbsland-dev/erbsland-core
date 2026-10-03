// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConstraintOptions.hpp"

#include "../../../../text/StringEditor.hpp"

namespace erbsland::conf::vr::builder {

using namespace text;
using namespace text::literals;

auto ConstraintOptions::prefixedConstraintName(const String &constraintName) const -> String {
    return withNotPrefix(constraintName, isNegated);
}

void ConstraintOptions::addToRule(
    RuleDefinition &rule, const vr::ConstraintPtr &constraint, const String &constraintName) const {
    rule.addConstraint(constraint, constraintName, isNegated, errorMessage);
}

auto ConstraintOptions::withNotPrefix(const String &name, const bool isNegated) -> String {
    if (!isNegated) {
        return name;
    }
    StringEditor result{"not_"_el};
    result.append(name);
    return result;
}

}
