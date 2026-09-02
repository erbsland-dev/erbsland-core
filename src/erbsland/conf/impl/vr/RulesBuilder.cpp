// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RulesBuilder.hpp"

#include "RulesFromDocument.hpp"
#include "ValidationError.hpp"

#include "../../ConfError.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

RulesBuilder::RulesBuilder() : _rules(std::make_shared<Rules>()) {
}

void RulesBuilder::configureRoot(const std::span<const vr::builder::Attribute *const> attributes) {
    for (const auto *attribute : attributes) {
        attribute->apply(*_rules->root());
    }
}

void RulesBuilder::addRule(
    const NamePathLike &namePath,
    const vr::RuleType ruleType,
    const std::span<const vr::builder::Attribute *const> attributes) {
    if (ruleType == vr::RuleType::Undefined) {
        throwValidationError("A rule type of 'undefined' is not allowed"_el);
    }
    auto ruleNamePath = resolveRuleNamePath(namePath);
    auto rule = std::make_shared<Rule>();
    rule->setRuleNamePath(ruleNamePath);
    rule->setTargetNamePath(ruleNamePath);
    rule->setType(ruleType);
    for (const auto *attribute : attributes) {
        attribute->apply(*rule);
    }
    _rules->addRule(rule);
}

void RulesBuilder::addAlternative(
    const NamePathLike &namePath,
    const vr::RuleType ruleType,
    const std::span<const vr::builder::Attribute *const> attributes) {
    if (ruleType == vr::RuleType::Undefined) {
        throwValidationError("A rule type of 'undefined' is not allowed"_el);
    }
    auto ruleNamePath = resolveRuleNamePath(namePath);
    auto rule = std::make_shared<Rule>();
    rule->setRuleNamePath(ruleNamePath);
    rule->setTargetNamePath(ruleNamePath);
    rule->setType(ruleType);
    for (const auto *attribute : attributes) {
        attribute->apply(*rule);
    }
    _rules->addAlternativeRule(rule);
}

void RulesBuilder::readFromDocument(const DocumentPtr &document) {
    RulesFromDocument rulesFromDocument{_rules, document};
    rulesFromDocument.read();
    _rules->validateDefinition();
}

auto RulesBuilder::resolveRuleNamePath(const NamePathLike &namePathLike) -> NamePath {
    NamePath namePath;
    if (std::holds_alternative<std::size_t>(namePathLike)) {
        throwValidationError("The given name-path is not valid"_el);
    }
    if (std::holds_alternative<text::String>(namePathLike)) {
        namePath = NamePath::fromText(std::get<text::String>(namePathLike));
    } else if (std::holds_alternative<NamePath>(namePathLike)) {
        namePath = std::get<NamePath>(namePathLike);
    } else if (std::holds_alternative<Name>(namePathLike)) {
        namePath = NamePath{{std::get<Name>(namePathLike)}};
    }
    if (namePath.empty()) {
        throwValidationError("An empty name-path is not valid"_el);
    }
    if (namePath.containsText() || namePath.containsIndex()) {
        throwValidationError("Text names or indexes are not allowed in a name-path for validation rules"_el);
    }
    return namePath;
}

void RulesBuilder::reset() {
    _rules = std::make_shared<Rules>();
}

auto RulesBuilder::takeRules() -> RulesPtr {
    _rules->validateDefinition();
    auto result = _rules;
    reset();
    return result;
}

}
