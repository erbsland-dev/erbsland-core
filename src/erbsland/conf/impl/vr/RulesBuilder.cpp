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
    const auto rule = _rules->root();
    auto entry = applyAttributes(rule, attributes);
    attachImplicitEntry(rule, entry);
}

void RulesBuilder::configureRule(
    const NamePath &namePath, const std::span<const vr::builder::Attribute *const> attributes) {
    const auto rule = _rules->ruleForNamePath(namePath);
    if (rule == nullptr) {
        throwValidationError("Cannot configure a missing branch parent rule."_el);
    }
    auto entry = applyAttributes(rule, attributes);
    attachImplicitEntry(rule, entry);
}

auto RulesBuilder::applyAttributes(const RulePtr &rule, const std::span<const vr::builder::Attribute *const> attributes)
    -> RulePtr {
    // Stage changes so a rejected entry declaration does not modify an existing branch parent.
    auto stagedRule = std::make_shared<Rule>(*rule);
    stagedRule->clearEntryRequest();
    for (const auto *attribute : attributes) {
        attribute->apply(*stagedRule);
    }
    auto entry = prepareImplicitEntry(stagedRule);
    stagedRule->clearEntryRequest();
    *rule = std::move(*stagedRule);
    return entry;
}

void RulesBuilder::addRule(
    const NamePathLike &namePath,
    const vr::RuleType ruleType,
    const std::span<const vr::builder::Attribute *const> attributes) {
    if (ruleType == vr::RuleType::Undefined) {
        throwValidationError("A rule type of 'undefined' is not allowed"_el);
    }
    auto ruleNamePath = resolveRuleNamePath(namePath);
    if (_rules->ruleForNamePath(ruleNamePath) != nullptr) {
        throwValidationError("A rule already exists at the given name path."_el);
    }
    auto rule = std::make_shared<Rule>();
    rule->setRuleNamePath(ruleNamePath);
    rule->setTargetNamePath(ruleNamePath);
    rule->setType(ruleType);
    auto entry = applyAttributes(rule, attributes);
    _rules->addRule(rule);
    attachImplicitEntry(rule, entry);
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
    auto entry = applyAttributes(rule, attributes);
    _rules->addAlternativeRule(rule);
    attachImplicitEntry(rule, entry);
}

auto RulesBuilder::prepareImplicitEntry(const RulePtr &rule) -> RulePtr {
    if (rule->entryType().isUndefined()) {
        if (rule->hasEntryAttributes()) {
            throwValidationError("Entry attributes require EntryType in the same declaration."_el);
        }
        return {};
    }
    if (rule->type() != vr::RuleType::ValueList || rule->hasChild(Name::vrName(Name::VR::ReservedEntry))) {
        throwValidationError("An implicit scalar entry requires a value list without an existing vr_entry."_el);
    }
    auto entry = std::make_shared<Rule>();
    entry->setType(rule->entryType());
    for (const auto &attribute : rule->entryAttributes()) {
        attribute->apply(*entry);
    }
    return entry;
}

void RulesBuilder::attachImplicitEntry(const RulePtr &rule, const RulePtr &entry) {
    if (entry == nullptr) {
        return;
    }
    auto rulePath = rule->ruleNamePath();
    rulePath.append(Name::vrName(Name::VR::ReservedEntry));
    auto targetPath = rule->targetNamePath();
    targetPath.append(Name::vrName(Name::VR::ReservedEntry));
    entry->setRuleNamePath(rulePath);
    entry->setTargetNamePath(targetPath);
    entry->setParent(rule);
    rule->addChild(entry);
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
