// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule.hpp"
#include "Rules.hpp"
#include "ValidationError.hpp"

#include "../../Document.hpp"
#include "../../vr/builder/Attributes.hpp"

namespace erbsland::conf::impl {

/// Build a validation-rule collection from documents or public builder calls.
class RulesBuilder {
public:
    /// Create an empty validation-rule builder.
    RulesBuilder();

    // defaults/deletions
    ~RulesBuilder() = default;
    RulesBuilder(const RulesBuilder &) = delete;
    auto operator=(const RulesBuilder &) -> RulesBuilder & = delete;
    RulesBuilder(RulesBuilder &&) = delete;
    auto operator=(RulesBuilder &&) -> RulesBuilder & = delete;

public:
    /// Replace the current rules with rules read from a configuration document.
    /// @param document The source configuration document.
    void readFromDocument(const DocumentPtr &document);
    /// Discard the current rules and begin an empty rules collection.
    void reset();
    /// Transfer the completed rules collection and reset the builder.
    /// @return The completed rules collection.
    [[nodiscard]] auto takeRules() -> RulesPtr;

public:
    /// Add a primary validation rule.
    /// @tparam Attributes The rule attribute types.
    /// @param namePath The rule name path.
    /// @param ruleType The type of values accepted by the rule.
    /// @param attributes The attributes that configure the rule.
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

    /// Add an alternative validation rule.
    /// @tparam Attributes The rule attribute types.
    /// @param namePath The rule name path.
    /// @param ruleType The type of values accepted by the rule.
    /// @param attributes The attributes that configure the rule.
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
    /// Resolve a rule name path to its canonical representation.
    /// @param namePathLike The source rule name path.
    /// @return The resolved name path.
    [[nodiscard]] auto resolveRuleNamePath(const NamePathLike &namePathLike) -> NamePath;

private:
    RulesPtr _rules; ///< The currently edited rules document.
};

}
