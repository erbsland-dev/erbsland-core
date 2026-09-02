// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Matches.hpp"

#include "../../../impl/vr/MatchesConstraint.hpp"
#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void Matches::apply(RuleDefinition &rule) const {
    requireRuleTypeForConstraint(rule, "matches"_el, {vr::RuleType::Text});
    if (_compiledPattern == nullptr && _pattern.isEmpty()) {
        throwValidationError("The regular expression in 'matches' constraint cannot be empty"_el);
    }
    auto constraint = _compiledPattern != nullptr ? std::make_shared<impl::MatchesConstraint>(_compiledPattern)
                                                  : std::make_shared<impl::MatchesConstraint>(_pattern, _isVerbose);
    _options.addToRule(rule, constraint, "matches"_el);
}

}
