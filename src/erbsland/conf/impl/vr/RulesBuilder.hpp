// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule.hpp"
#include "Rules.hpp"
#include "ValidationError.hpp"

#include "../../Document.hpp"
#include "../../vr/builder/Attributes.hpp"

namespace erbsland::conf::impl {

class RulesBuilder {
public:
    RulesBuilder();
    ~RulesBuilder() = default;
    RulesBuilder(const RulesBuilder &) = delete;
    auto operator=(const RulesBuilder &) -> RulesBuilder & = delete;
    RulesBuilder(RulesBuilder &&) = delete;
    auto operator=(RulesBuilder &&) -> RulesBuilder & = delete;

public:
    void readFromDocument(const DocumentPtr &document);
    void reset();
    [[nodiscard]] auto takeRules() -> RulesPtr;

public:
    template <typename... Attributes>
        requires(std::derived_from<Attributes, vr::builder::Attribute> && ...)
    void addRule(const NamePathLike &namePath, const vr::RuleType ruleType, Attributes... attributes) {

        if (ruleType == vr::RuleType::Undefined) {
            throwValidationError("A rule type of 'undefined' is not allowed"_el);
        }
        auto ruleNamePath = resolveRuleNamePath(namePath);
        auto rule = std::make_shared<Rule>();
        rule->setRuleNamePath(ruleNamePath);
        rule->setTargetNamePath(ruleNamePath);
        rule->setType(ruleType);
        (attributes(*rule), ...);
        _rules->addRule(rule);
    }

    template <typename... Attributes>
        requires(std::derived_from<Attributes, vr::builder::Attribute> && ...)
    void addAlternative(const NamePathLike &namePath, const vr::RuleType ruleType, Attributes... attributes) {

        if (ruleType == vr::RuleType::Undefined) {
            throwValidationError("A rule type of 'undefined' is not allowed"_el);
        }
        auto ruleNamePath = resolveRuleNamePath(namePath);
        auto rule = std::make_shared<Rule>();
        rule->setRuleNamePath(ruleNamePath);
        rule->setTargetNamePath(ruleNamePath);
        rule->setType(ruleType);
        (attributes(*rule), ...);
        _rules->addAlternativeRule(rule);
    }

private:
    [[nodiscard]] auto resolveRuleNamePath(const NamePathLike &namePathLike) -> NamePath;

private:
    RulesPtr _rules; ///< The currently edited rules document.
};

}
