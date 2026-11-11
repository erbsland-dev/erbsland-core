// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RulesBuilder.hpp"

#include "RulesBuilderBranch.hpp"

#include "../impl/vr/RulesBuilder.hpp"

#include "../../err/ParameterFailure.hpp"

#include <utility>

namespace erbsland::conf::vr {

RulesBuilder::RulesBuilder() : _builder{std::make_shared<impl::RulesBuilder>()} {
}

RulesBuilder::RulesBuilder(std::shared_ptr<impl::RulesBuilder> builder) : _builder{std::move(builder)} {
    if (_builder == nullptr) {
        using namespace text::literals;
        throw err::ParameterFailure{"The validation-rule builder backend cannot be null."_el, "builder"_el};
    }
}

RulesBuilder::~RulesBuilder() = default;

auto RulesBuilder::parentPath() const -> NamePath {
    return {};
}

auto RulesBuilder::createBranch(const NamePathLike &namePath) -> RulesBuilderBranch {
    auto path = impl::RulesBuilder::resolveDefinitionNamePath(namePath);
    path.prepend(parentPath());
    return RulesBuilderBranch{*this, std::move(path)};
}

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
    resetImpl();
}

void RulesBuilder::resetImpl() {
    _builder->reset();
}

auto RulesBuilder::takeRules() -> RulesPtr {
    return takeRulesImpl();
}

auto RulesBuilder::takeRulesImpl() -> RulesPtr {
    return _builder->takeRules();
}

}
