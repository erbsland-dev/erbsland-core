// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "DependencyDefinition.hpp"
#include "KeyDefinition.hpp"
#include "Rule_fwd.hpp"
#include "RuleMap.hpp"
#include "RulesConstants.hpp"
#include "VersionMask.hpp"

#include "../lexer/Content.hpp"

#include "../../../text/CaseSensitivity.hpp"
#include "../../../text/String.hpp"
#include "../../Name.hpp"
#include "../../vr/Constraint.hpp"
#include "../../vr/Rule.hpp"
#include "../../vr/RuleType.hpp"

namespace erbsland::conf::impl {

/// Internal implementation of a validation rule.
/// @tested{VrNodeRulesDefinitionTest}
class Rule : public vr::Rule {
public:
    // defaults
    Rule() = default;
    ~Rule() override = default;

public: // public interface
    [[nodiscard]] auto namePath() const -> NamePath override { return _targetNamePath; }
    [[nodiscard]] auto type() const -> vr::RuleType override { return _type; }
    [[nodiscard]] auto title() const -> text::String override { return _title; }
    [[nodiscard]] auto description() const -> text::String override { return _description; }
    [[nodiscard]] auto hasDefault() const -> bool override { return _defaultValue != nullptr; }
    [[nodiscard]] auto hasCustomError() const -> bool override { return !_errorMessage.isEmpty(); }
    [[nodiscard]] auto customError() const -> text::String override { return _errorMessage; }
    [[nodiscard]] auto constraints() const -> std::vector<vr::ConstraintPtr> override {
        std::vector<vr::ConstraintPtr> result;
        result.reserve(_constraints.size());
        for (const auto &constraint : _constraints) {
            result.push_back(constraint);
        }
        return result;
    }
    [[nodiscard]] auto isOptional() const -> bool override { return _isOptional; }
    [[nodiscard]] auto caseSensitivity() const -> text::CaseSensitivity override { return _caseSensitivity; }
    [[nodiscard]] auto isSecret() const -> bool override { return _isSecret; }
    [[nodiscard]] auto children() const -> std::vector<vr::RulePtr> override {
        std::vector<vr::RulePtr> result;
        result.reserve(_children.size());
        for (const auto &child : _children) {
            result.push_back(child);
        }
        return result;
    }
    [[nodiscard]] auto location() const noexcept -> const Location & override { return _location; }
    [[nodiscard]] auto hasLocation() const noexcept -> bool override { return !_location.isUndefined(); }
    void setLocation(const Location &location) noexcept override { _location = location; }

    // impl interface
    /// Get the name path at which this rule was declared.
    [[nodiscard]] auto ruleNamePath() const -> const NamePath & { return _ruleNamePath; }
    /// Get the final name of the rule declaration, or an empty name for the root.
    [[nodiscard]] auto ruleName() const -> const Name & {
        return _ruleNamePath.empty() ? Name::emptyInstance() : _ruleNamePath.back();
    }
    /// Get the name path that this rule validates.
    [[nodiscard]] auto targetNamePath() const -> const NamePath & { return _targetNamePath; }
    /// Get the final name of the validation target, or an empty name for the root.
    [[nodiscard]] auto targetName() const -> const Name & {
        return _targetNamePath.empty() ? Name::emptyInstance() : _targetNamePath.back();
    }
    /// Get the configured default value.
    [[nodiscard]] auto defaultValue() const -> const ValuePtr & { return _defaultValue; }
    /// Get the internal child-rule map.
    [[nodiscard]] auto childrenImpl() const -> const RuleMap & { return _children; }
    /// Get the parent rule, if one exists.
    [[nodiscard]] auto parent() const -> RulePtr { return _parent.lock(); }
    /// Set the name path at which this rule was declared.
    void setRuleNamePath(const NamePath &namePath) { _ruleNamePath = namePath; }
    /// Set the name path that this rule validates.
    void setTargetNamePath(const NamePath &namePath) { _targetNamePath = namePath; }
    /// Set the rule type.
    void setType(const vr::RuleType type) { _type = type; }
    /// Set the optional title.
    void setTitle(text::String &&title) noexcept { _title = std::move(title); }
    /// Set the optional title.
    void setTitle(const text::String &title) noexcept { _title = title; }
    /// Set the optional description.
    void setDescription(text::String &&description) noexcept { _description = std::move(description); }
    /// Set the optional description.
    void setDescription(const text::String &description) noexcept { _description = description; }
    /// Set the custom validation error message.
    void setErrorMessage(text::String &&errorMessage) noexcept { _errorMessage = std::move(errorMessage); }
    /// Set the custom validation error message.
    void setErrorMessage(const text::String &errorMessage) noexcept { _errorMessage = errorMessage; }
    /// Set whether the target value is optional.
    void setOptional(bool isOptional) { _isOptional = isOptional; }
    /// Set how text constraints compare characters.
    void setCaseSensitivity(const text::CaseSensitivity caseSensitivity) { _caseSensitivity = caseSensitivity; }
    /// Set whether the target value contains secret data.
    void setSecret(bool isSecret) { _isSecret = isSecret; }
    /// Set the default value.
    void setDefaultValue(const ValuePtr &value) { _defaultValue = value; }
    /// Add a constraint, replacing one of the same type if present.
    void addOrOverwriteConstraint(const ConstraintPtr &constraint);
    /// Test whether a constraint of the given type exists.
    [[nodiscard]] auto hasConstraint(vr::ConstraintType type) const -> bool;
    /// Test whether a constraint with the given name exists.
    [[nodiscard]] auto hasConstraint(const text::String &name) const -> bool;
    /// Get a constraint by name.
    [[nodiscard]] auto constraint(const text::String &name) const -> ConstraintPtr;
    /// Get a constraint by type.
    [[nodiscard]] auto constraint(vr::ConstraintType type) const -> ConstraintPtr;
    /// Get the internal constraint list.
    [[nodiscard]] auto constraintsImpl() const -> const ConstraintList & { return _constraints; }
    /// Test whether this rule has reserved-name constraints.
    [[nodiscard]] auto hasNameConstraints() const -> bool {
        return _children.hasRule(Name::vrName(Name::VR::ReservedName));
    }
    /// Get the reserved-name constraints.
    [[nodiscard]] auto nameConstraints() const -> RulePtr {
        return _children.rule(Name::vrName(Name::VR::ReservedName));
    }
    /// Add a key definition to this rule.
    void addKeyDefinition(const KeyDefinitionPtr &keyDefinition);
    /// Test whether this rule has key definitions.
    [[nodiscard]] auto hasKeyDefinitions() const -> bool;
    /// Get the key definitions.
    [[nodiscard]] auto keyDefinitions() const -> const KeyDefinitionList &;
    /// Test whether this rule has dependency definitions.
    [[nodiscard]] auto hasDependencyDefinitions() const -> bool { return !_dependencyDefinitions.empty(); }
    /// Get the dependency definitions.
    [[nodiscard]] auto dependencyDefinitions() const -> const DependencyDefinitionList & {
        return _dependencyDefinitions;
    }
    /// Add a dependency definition to this rule.
    void addDependencyDefinition(const DependencyDefinitionPtr &dependencyDefinition);
    /// Restrict the rule to versions permitted by the supplied mask.
    void limitVersionMask(const VersionMask &versionMask) { _versionMask &= versionMask; }
    /// Get the versions for which this rule applies.
    [[nodiscard]] auto versionMask() const -> const VersionMask & { return _versionMask; }
    /// Set the parent rule.
    void setParent(const RulePtr &parent) { _parent = RuleWeakPtr{parent}; }
    /// Test whether this rule has child rules.
    [[nodiscard]] auto empty() const -> bool { return _children.empty(); }
    /// Test whether a direct child with the given name exists.
    [[nodiscard]] auto hasChild(const Name &name) const -> bool { return _children.hasRule(name); }
    /// Get a direct child by name.
    [[nodiscard]] auto child(const Name &name) const -> RulePtr;
    /// Get a nested child by name path.
    [[nodiscard]] auto child(const NamePath &namePath) const -> RulePtr;
    /// Add a child rule.
    void addChild(const RulePtr &child) { _children.addRule(child); }

public: // tests
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const Rule &rule) -> InternalViewPtr;
    friend auto internalView(const RulePtr &rule) -> InternalViewPtr;
#endif

private:
    Location _location;                              ///< The source location of a rule
    NamePath _ruleNamePath;                          ///< The name path of the rule definition.
    NamePath _targetNamePath;                        ///< The path that points to the entry in the validated document.
    vr::RuleType _type;                              ///< The type of the rule.
    text::String _title;                             ///< A title for this rule.
    text::String _description;                       ///< A description of the rule.
    text::String _errorMessage;                      ///< A custom error message.
    bool _isOptional{false};                         ///< If this rule is optional.
    text::CaseSensitivity _caseSensitivity{
        text::CaseSensitivity::CaseInsensitive};     ///< If text is compared case-sensitive.
    bool _isSecret{false};                           ///< If this rule handles a secret value.
    ValuePtr _defaultValue;                          ///< An optional default value for this rule.
    ConstraintList _constraints;                     ///< A list of constraints.
    KeyDefinitionList _keyDefinitions;               ///< A list of key definitions.
    DependencyDefinitionList _dependencyDefinitions; ///< A list of dependency definitions.
    VersionMask _versionMask;                        ///< The version mask for this rule.
    RuleWeakPtr _parent;                             ///< The parent rule.
    RuleMap _children;                               ///< A map of child rules.
};

}
