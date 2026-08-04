// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Chars.hpp"

#include "../../../impl/vr/CharsConstraint.hpp"
#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void Chars::operator()(Rule &rule) {
    requireRuleTypeForConstraint(rule, "chars"_el, {vr::RuleType::Text});
    if (_values.isEmpty()) {
        throwValidationError("The 'chars' constraint must specify a single text value or a list of texts"_el);
    }
    auto constraint = std::make_shared<impl::CharsConstraint>(_values);
    _options.addToRule(rule, constraint, "chars"_el);
}

}
