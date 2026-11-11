// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RulesBuilderBranch.hpp"

#include "../impl/vr/RulesBuilder.hpp"

#include "../../err/LogicFailure.hpp"

#include <utility>

namespace erbsland::conf::vr {

using namespace text::literals;

RulesBuilderBranch::RulesBuilderBranch(const RulesBuilder &parent, NamePath parentPath) :
    RulesBuilder{parent._builder}, _parentPath{std::move(parentPath)} {
}

auto RulesBuilderBranch::parentPath() const -> NamePath {
    return _parentPath;
}

auto RulesBuilderBranch::resolvePath(const NamePathLike &namePath) const -> NamePath {
    auto path = impl::RulesBuilder::resolveDefinitionNamePath(namePath);
    path.prepend(_parentPath);
    return path;
}

void RulesBuilderBranch::configureRootImpl(const std::span<const builder::Attribute *const> attributes) {
    _builder->configureRule(_parentPath, attributes);
}

void RulesBuilderBranch::addRuleImpl(
    const NamePathLike &namePath,
    const RuleType ruleType,
    const std::span<const builder::Attribute *const> attributes) {
    RulesBuilder::addRuleImpl(resolvePath(namePath), ruleType, attributes);
}

void RulesBuilderBranch::addAlternativeImpl(
    const NamePathLike &namePath,
    const RuleType ruleType,
    const std::span<const builder::Attribute *const> attributes) {
    RulesBuilder::addAlternativeImpl(resolvePath(namePath), ruleType, attributes);
}

void RulesBuilderBranch::resetImpl() {
    throw err::LogicFailure{"Cannot reset a validation-rule builder branch."_el};
}

auto RulesBuilderBranch::takeRulesImpl() -> RulesPtr {
    throw err::LogicFailure{"Cannot take rules from a validation-rule builder branch."_el};
}

}
