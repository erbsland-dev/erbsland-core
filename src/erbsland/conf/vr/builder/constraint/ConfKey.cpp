// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfKey.hpp"

#include "../NamePathHelper.hpp"

#include "../../../impl/vr/KeyConstraint.hpp"
#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void ConfKey::operator()(impl::Rule &rule) {
    requireRuleTypeForConstraint(rule, "key"_el, {vr::RuleType::Text, vr::RuleType::Integer});
    auto references = detail::parseNamePathList(_references);
    auto constraint = std::make_shared<impl::KeyConstraint>(std::move(references));
    _options.addToRule(rule, constraint, "key"_el);
}

}
