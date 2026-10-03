// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule.hpp"
#include "Rules.hpp"
#include "RulesBuilder_fwd.hpp"
#include "ValidationError.hpp"

#include "../../Document.hpp"
#include "../../vr/builder/Attributes.hpp"

#include <span>

namespace erbsland::conf::impl {

/// Build a validation-rule collection from documents or public builder calls.
/// @tested{VrBuilderApiTest VrBuilderParityTest VrEntryTypeTest}
class RulesBuilder {
public: // programmatic construction
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
    /// Configure attributes of the implicit root section rule.
    /// @param attributes The attributes that configure the root rule.
    void configureRoot(std::span<const vr::builder::Attribute *const> attributes);

    /// Add a primary validation rule.
    /// @param namePath The rule name path.
    /// @param ruleType The type of values accepted by the rule.
    /// @param attributes The attributes that configure the rule.
    void addRule(
        const NamePathLike &namePath, vr::RuleType ruleType, std::span<const vr::builder::Attribute *const> attributes);

    /// Add an alternative validation rule.
    /// @param namePath The rule name path.
    /// @param ruleType The type of values accepted by the rule.
    /// @param attributes The attributes that configure the rule.
    void addAlternative(
        const NamePathLike &namePath, vr::RuleType ruleType, std::span<const vr::builder::Attribute *const> attributes);

public: // branch construction
    /// Apply attributes to an existing rule at a complete path.
    /// @param namePath The complete rule path.
    /// @param attributes The attributes to apply.
    void configureRule(const NamePath &namePath, std::span<const vr::builder::Attribute *const> attributes);
    /// Resolve a rule name path to its canonical representation.
    /// @param namePathLike The source rule name path.
    /// @return The resolved name path.
    [[nodiscard]] static auto resolveRuleNamePath(const NamePathLike &namePathLike) -> NamePath;

private:
    /// Apply attributes and validate an implicit entry before modifying the rule tree.
    static auto applyAttributes(const RulePtr &rule, std::span<const vr::builder::Attribute *const> attributes)
        -> RulePtr;
    /// Prepare the requested scalar entry without attaching it to the rule tree.
    static auto prepareImplicitEntry(const RulePtr &rule) -> RulePtr;
    /// Attach a prepared entry after its parent has its final declaration path.
    static void attachImplicitEntry(const RulePtr &rule, const RulePtr &entry);

private:
    RulesPtr _rules; ///< The currently edited rules document.
};

}
