// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyIndex.hpp"
#include "Rule.hpp"

#include "../value/Value.hpp"

#include <vector>

namespace erbsland::conf::impl {

/// The document validator.
/// Used by the validation rules to validate documents and value trees.
class DocumentValidator final {
private:
    /// Frame for the validation stack.
    struct Frame {
        conf::ValuePtr valueNode;
        RulePtr ruleNode;
    };

public:
    /// Create a new validator instance.
    /// @param root The root of the rule tree for validation.
    /// @param value The root value to validate.
    /// @param version The version of the document format to validate.
    DocumentValidator(RulePtr root, conf::ValuePtr value, Integer version);

    // defaults and deletions
    ~DocumentValidator() = default;
    DocumentValidator(const DocumentValidator &) = delete;
    DocumentValidator(DocumentValidator &&) = delete;
    auto operator=(const DocumentValidator &) -> DocumentValidator & = delete;
    auto operator=(DocumentValidator &&) -> DocumentValidator & = delete;

public:
    /// Validate the rules against the given value tree.
    void validate();

private:
    /// The first pass of validation.
    /// Validates everything, except keys and dependencies.
    void validatePass1();

    /// The second pass of validation.
    /// Validates keys and dependencies.
    void validatePass2();

    /// Validate the given value against the given rule.
    /// @param rule The rule to validate against.
    /// @param value The value to validate.
    /// @return The next parent rule to validate the child values against, or nullptr if no children
    ///     should be validated (because the branch is marked as not validated).
    [[nodiscard]] auto validate(const RulePtr &rule, const ValuePtr &value) -> RulePtr;

    /// Handle missing values.
    /// @param rule The rule that has a missing value.
    /// @param parentValue The parent value where the value is missing.
    ///     This can be any node, including the document itself.
    void handleMissingValues(const RulePtr &rule, const conf::ValuePtr &parentValue);

    /// Copy default value.
    /// @param rule The rule with the default value to copy.
    /// @param parentValue The parent value where the default value should be added.
    ///     This can be any node, including the document itself.
    void copyDefaultValue(const RulePtr &rule, const conf::ValuePtr &parentValue);

    /// Handle unvalidated values.
    /// @param rule The rule to validate against.
    /// @param value The value to validate.
    [[nodiscard]] auto handleNotValidatedValues(const RulePtr &rule, const ValuePtr &value) -> RulePtr;

    /// Handle alternatives.
    /// @param rule The rule to validate against.
    /// @param value The value to validate.
    [[nodiscard]] auto handleAlternatives(const RulePtr &rule, const ValuePtr &value) -> RulePtr;

    /// Handle section lists.
    /// @param rule The rule to validate against.
    /// @param value The value to validate.
    [[nodiscard]] auto handleSectionLists(const RulePtr &rule, const ValuePtr &value) -> RulePtr;

    /// Handle value lists or matrices.
    /// @param rule The rule to validate against.
    /// @param value The value to validate.
    /// @return The value rule used to validate the individual values.
    [[nodiscard]] auto handleValueListOrMatrixPreCheck(const RulePtr &rule, const ValuePtr &value) -> RulePtr;

    /// Validate a single value is a list or matrix.
    /// @param valueRule The rule used to validate the value.
    /// @param value The value to validate.
    void validateListOrMatrixValue(const RulePtr &valueRule, const ValuePtr &value);

    /// Handle value lists.
    /// @param rule The rule to validate against.
    /// @param value The value to validate.
    [[nodiscard]] auto handleValueLists(const RulePtr &rule, const ValuePtr &value) -> RulePtr;

    /// Handle value matrices.
    /// @param rule The rule to validate against.
    /// @param value The value to validate.
    [[nodiscard]] auto handleValueMatrix(const RulePtr &rule, const ValuePtr &value) -> RulePtr;

    /// Handle regular sections and scalar values.
    /// @param rule The rule to validate against.
    /// @param value The value to validate.
    [[nodiscard]] auto handleCommonValues(const RulePtr &rule, const ValuePtr &value) -> RulePtr;

    /// Build all key indexes for a given rule.
    /// @param value The value node that is entered.
    /// @param rule The rule assigned to this node.
    [[nodiscard]] auto buildKeyIndexes(const conf::ValuePtr &value, const RulePtr &rule) -> KeyIndexList;

    /// Build a key index and validate the uniqueness of all values.
    /// @param value The value node that is entered which is the base node for the index.
    /// @param keyDefinition The key definition that is used to build the index.
    /// @return The key index that was built.
    /// @throws ConfError if the key index is invalid, or if the key values are not unique.
    [[nodiscard]] auto buildKeyIndexAndValidateUniqueness(
        const conf::ValuePtr &value, const KeyDefinitionPtr &keyDefinition) -> KeyIndexPtr;

    /// Validate a key constraint.
    /// @param indexStack The stack of key indexes.
    /// @param value The validated value.
    /// @param rule The rule that contains the key constraint.
    void validateKeyConstraint(const KeyIndexList &indexStack, const conf::ValuePtr &value, const RulePtr &rule);

    /// Handle dependency checks when entering a value node.
    /// @param value The value node that is entered.
    /// @param rule The rule assigned to this node.
    void validateDependencies(const conf::ValuePtr &value, const RulePtr &rule);

    /// Select the matching rule for the given value.
    /// @param parentRule The parent rule that contains child-rules that should match `value`.
    /// @param value The value for which a suitable rule shall be found.
    /// @return The matching child rule. Never nullptr - will throw on failure.
    [[nodiscard]] auto nextRuleForValue(const RulePtr &parentRule, const ValuePtr &value) const -> RulePtr;

    /// Validate the name constraints of a rule for the name of a given value.
    /// @param rule The rule with the name constraints.
    /// @param value The value for which the name constraints shall be validated.
    static void validateNameConstraints(const RulePtr &rule, const ValuePtr &value);

    /// Validate the main constraints of a rule.
    /// @param rule The rule to validate.
    /// @param value The value to validate against the rule.
    void validateValueConstraints(const RulePtr &rule, const ValuePtr &value);

    /// Validate the actual constraints of the rule using a given validation context.
    static void validateConstraints(const RulePtr &rule, const ValidationContext &validationContext);

    /// Get a textual representation of the expected value type, based on the given rule.
    /// @param rule The rule.
    /// @return The textual representation of the expected value type.
    [[nodiscard]] auto expectedValueTypeText(const RulePtr &rule) const -> text::String;

    /// Get an error text description for the parent location, based on a value.
    [[nodiscard]] static auto parentLocationText(const conf::ValuePtr &value) -> text::String;

    /// Throw an error showing the expected and actual value for a given rule.
    [[noreturn]] void throwExpectedVsActual(const RulePtr &rule, const ValuePtr &value) const;

    /// Get a list of paths for an error message.
    [[nodiscard]] static auto errorNamePathsOr(const NamePathList &paths, bool forNegation) -> text::String;

private:
    RulePtr _root;                ///< The root node for the rule-tree.
    conf::ValuePtr _value;        ///< The root node for the value-tree.
    Integer _version{0};          ///< The user-selected version for the validation.
    bool _useIndexes{false};      ///< True if the validation-rules make use of indexes.
    bool _useDependencies{false}; ///< True if the validation-rules make use of dependencies.
};

}
