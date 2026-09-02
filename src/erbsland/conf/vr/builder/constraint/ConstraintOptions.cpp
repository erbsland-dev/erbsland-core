// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConstraintOptions.hpp"

#include "../../../../text/StringEditor.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

auto ConstraintOptions::prefixedConstraintName(const text::String &constraintName) const -> text::String {
    return withNotPrefix(constraintName, isNegated);
}

void ConstraintOptions::addToRule(
    RuleDefinition &rule, const vr::ConstraintPtr &constraint, const text::String &constraintName) const {
    rule.addConstraint(constraint, constraintName, isNegated, errorMessage);
}

auto ConstraintOptions::withNotPrefix(const text::String &name, const bool isNegated) -> text::String {
    if (!isNegated) {
        return name;
    }
    text::StringEditor result{"not_"_el};
    result.append(name);
    return result;
}

}
