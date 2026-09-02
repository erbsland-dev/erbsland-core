// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfKey.hpp"

#include "../../../impl/vr/KeyConstraint.hpp"
#include "../../../impl/vr/NamePathHelper.hpp"
#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void ConfKey::apply(RuleDefinition &rule) const {
    requireRuleTypeForConstraint(rule, "key"_el, {vr::RuleType::Text, vr::RuleType::Integer});
    auto references = impl::parseNamePathList(_references);
    auto constraint = std::make_shared<impl::KeyConstraint>(std::move(references));
    _options.addToRule(rule, constraint, "key"_el);
}

}
