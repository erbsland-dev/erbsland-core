// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RulesBuilder.hpp"

#include "../impl/vr/RulesBuilder.hpp"

namespace erbsland::conf::vr {

RulesBuilder::RulesBuilder() : _builder{std::make_unique<impl::RulesBuilder>()} {
}

RulesBuilder::~RulesBuilder() = default;

void RulesBuilder::configureRootImpl(const std::span<const builder::Attribute *const> attributes) {
    _builder->configureRoot(attributes);
}

void RulesBuilder::addRuleImpl(
    const NamePathLike &namePath,
    const RuleType ruleType,
    const std::span<const builder::Attribute *const> attributes) {
    _builder->addRule(namePath, ruleType, attributes);
}

void RulesBuilder::addAlternativeImpl(
    const NamePathLike &namePath,
    const RuleType ruleType,
    const std::span<const builder::Attribute *const> attributes) {
    _builder->addAlternative(namePath, ruleType, attributes);
}

void RulesBuilder::reset() {
    _builder->reset();
}

auto RulesBuilder::takeRules() -> RulesPtr {
    return _builder->takeRules();
}

}
