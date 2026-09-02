// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringPartConstraint.hpp"

#include "../../../../text/StringFormat.hpp"
#include "../../../impl/vr/ContainsConstraint.hpp"
#include "../../../impl/vr/EndsConstraint.hpp"
#include "../../../impl/vr/StartsConstraint.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void StringPartConstraint::apply(RuleDefinition &rule) const {
    text::String name;
    vr::ConstraintPtr constraint;
    switch (_kind) {
    case Kind::Starts:
        name = "starts"_el;
        constraint = std::make_shared<impl::StartsConstraint>(_values);
        break;
    case Kind::Ends:
        name = "ends"_el;
        constraint = std::make_shared<impl::EndsConstraint>(_values);
        break;
    case Kind::Contains:
        name = "contains"_el;
        constraint = std::make_shared<impl::ContainsConstraint>(_values);
        break;
    }
    requireRuleTypeForConstraint(rule, name, {vr::RuleType::Text});
    if (_values.isEmpty()) {
        throwValidationError(
            text::StringFormat{"The '{}' constraint must specify a single text value or a list of texts"_el}.build(
                name));
    }
    _options.addToRule(rule, constraint, name);
}

}
