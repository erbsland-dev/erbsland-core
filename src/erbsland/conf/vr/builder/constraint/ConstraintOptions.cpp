// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConstraintOptions.hpp"

#include "../../../../text/StringEditor.hpp"
#include "../../../impl/vr/Constraint.hpp"
#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

auto ConstraintOptions::prefixedConstraintName(const text::String &constraintName) const -> text::String {
    return withNotPrefix(constraintName, isNegated);
}

void ConstraintOptions::applyTo(impl::Constraint &constraint, const text::String &constraintName) const {
    constraint.setName(prefixedConstraintName(constraintName));
    constraint.setNegated(isNegated);
    if (!errorMessage.isEmpty()) {
        constraint.setErrorMessage(errorMessage);
    }
}

void ConstraintOptions::addToRule(
    impl::Rule &rule, const impl::ConstraintPtr &constraint, const text::String &constraintName) const {
    applyTo(*constraint, constraintName);
    rule.addOrOverwriteConstraint(constraint);
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
