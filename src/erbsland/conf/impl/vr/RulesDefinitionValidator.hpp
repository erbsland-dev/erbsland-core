// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule.hpp"

#include <unordered_set>
#include <vector>

namespace erbsland::conf::impl {

/// Validate rule-tree relationships and finalize key paths for document validation.
/// @tested{VrRulePathsTest VrLiteralNamesTest VrLiteralPathsTest}
class RulesDefinitionValidator final {
public:
    /// Create a validator for a complete, unfinished rule tree.
    /// @param root The non-null root of the expanded rule tree.
    explicit RulesDefinitionValidator(RulePtr root);

    // defaults
    ~RulesDefinitionValidator() = default;

public:
    /// Validate the rule tree definition.
    /// @throws ConfError (Validation) if any issues are found in the tree.
    void validate();

private:
    /// Validate a rule in the rule tree.
    void validateRule(const RulePtr &rule);
    /// Reject duplicate literal target names among sibling rules.
    static void testUniqueValueNames(const RulePtr &rule);
    /// Test alternatives related constraints.
    static void testAlternatives(const RulePtr &rule);
    /// Test `vr_any` restrictions.
    static void testVrAny(const RulePtr &rule);
    /// Test if a `vr_name` definition has type text.
    static void testVrNameMustBeText(const RulePtr &rule);
    /// Test section-list related constraints.
    static void testSectionList(const RulePtr &rule);
    /// Test value list related constraints.
    static void testValueList(const RulePtr &rule);
    /// Test common constraints of `vr_entry`.
    static void testVrEntryCommonConstraints(const RulePtr &rule);
    /// Test for no other subsection in list definitions.
    static void testNoOtherSubsectionInListDefinitions(const RulePtr &rule);
    /// Test defaults and optionality.
    static void testDefaultsAndOptionality(const RulePtr &rule);
    /// Test if secret marker is used on allowed rule types.
    static void testSecretMarkerType(const RulePtr &rule);
    /// Test if minimum and maximum constraints define a valid range.
    static void testMinimumMaximumRelation(const RulePtr &rule);
    /// Validate key definitions and publish list and entry-relative paths after successful resolution.
    static void testKeyDefinitionPlacement(const RulePtr &rule);
    /// Test one key definition.
    static void validateKeyReference(const RulePtr &rule, const NamePath &keyReference);
    /// Test the key references.
    static void testKeyReferences(const RulePtr &rule);
    /// Resolve all possible types of one component of a key definition.
    static auto resolveKeyDefinitionType(const RulePtr &rule, const KeyDefinitionPtr &keyDefinition, std::size_t index)
        -> std::vector<vr::RuleType>;
    /// Test a `vr_dependency` definition.
    static void testDependencyDefinition(const RulePtr &rule);
    /// Resolve a relative value path and check ancestors below its owning rule.
    /// List-entry crossings are forbidden; the owning rule cannot supply optionality.
    static void validateDependencyPath(const RulePtr &rule, const NamePath &dependencyPath);
    /// Test if a rule is optional.
    [[nodiscard]] static auto isRuleOptional(const RulePtr &rule) -> bool;

private:
    RulePtr _root; ///< The root of the rule tree to validate.
};

}
