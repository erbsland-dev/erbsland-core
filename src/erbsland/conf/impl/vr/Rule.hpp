// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "DependencyDefinition.hpp"
#include "KeyDefinition.hpp"
#include "Rule_fwd.hpp"
#include "RuleMap.hpp"
#include "RuleRole.hpp"
#include "RulesConstants.hpp"
#include "VersionMask.hpp"

#include "../lexer/Content.hpp"

#include "../../../text/CaseSensitivity.hpp"
#include "../../../text/String.hpp"
#include "../../Name.hpp"
#include "../../vr/builder/RuleDefinition.hpp"
#include "../../vr/Constraint.hpp"
#include "../../vr/Rule.hpp"
#include "../../vr/RuleType.hpp"

namespace erbsland::conf::impl {

/// Internal implementation of a validation rule.
/// @tested{VrNodeRulesDefinitionTest VrRulePathsTest VrLiteralNamesTest}
class Rule : public vr::Rule, public vr::builder::RuleDefinition {
public:
    /// Create an unfinished rule with an explicit role.
    /// @param role The role determined from the declaration before escape decoding.
    explicit Rule(RuleRole role) : _role{role} {}

    // defaults
    ~Rule() override = default;

public: // public interface
    [[nodiscard]] auto namePath() const -> NamePath override { return _valueNamePath; }
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

public: // impl interface
    /// Get this node's purpose, independently of its value type and spelling.
    [[nodiscard]] auto role() const noexcept -> RuleRole { return _role; }
    /// Change the role before attaching the rule as an alternative branch.
    void setRole(RuleRole role) noexcept { _role = role; }
    /// Get the absolute path in the expanded rule tree, retaining escapes and alternative indexes.
    /// Template paths identify the insertion point; location() retains the source declaration location.
    [[nodiscard]] auto definitionNamePath() const -> const NamePath & { return _definitionNamePath; }
    /// Get the final definition name or alternative index, or an empty name for the root.
    [[nodiscard]] auto definitionName() const -> const Name & {
        if (_definitionNamePath.empty()) {
            return Name::emptyInstance();
        }
        return _definitionNamePath.back();
    }
    /// Get the absolute symbolic value path, with decoded names and without alternative indexes.
    /// Entry, wildcard and name-constraint components are placeholders, not concrete document names.
    /// Alternative branches share their container's path; only the root has an empty path.
    [[nodiscard]] auto valueNamePath() const -> const NamePath & { return _valueNamePath; }
    /// Get the final symbolic value name, or an empty name for the root.
    [[nodiscard]] auto valueName() const -> const Name & {
        if (_valueNamePath.empty()) {
            return Name::emptyInstance();
        }
        return _valueNamePath.back();
    }
    /// Get the configured default value.
    [[nodiscard]] auto defaultValue() const -> const ValuePtr & { return _defaultValue; }
    /// Get children in declaration order, indexed by definition name, never by value name.
    [[nodiscard]] auto childrenImpl() const -> const RuleMap & { return _children; }
    /// Get the parent rule, if one exists.
    [[nodiscard]] auto parent() const -> RulePtr { return _parent.lock(); }
    /// Set the expanded definition path before attaching this node to its parent.
    void setDefinitionNamePath(const NamePath &namePath) { _definitionNamePath = namePath; }
    /// Set the symbolic value path during construction; callers perform document escape decoding.
    void setValueNamePath(const NamePath &namePath) { _valueNamePath = namePath; }
    /// Set the rule type.
    void setType(const vr::RuleType type) override { _type = type; }
    /// Set the requested implicit entry type.
    void setEntryType(vr::RuleType type) override;
    void addEntryAttributes(std::vector<std::shared_ptr<const vr::builder::Attribute>> attributes) override;
    /// Get the queued entry attributes for this builder call.
    [[nodiscard]] auto entryAttributes() const -> const std::vector<std::shared_ptr<const vr::builder::Attribute>> & {
        return _entryAttributes;
    }
    /// Test whether entry attributes were requested, including an empty constraint pack.
    [[nodiscard]] auto hasEntryAttributes() const -> bool { return _hasEntryAttributes; }
    /// Clear the temporary entry request after a builder call.
    void clearEntryRequest() {
        _entryType = vr::RuleType::Undefined;
        _entryAttributes.clear();
        _hasEntryAttributes = false;
    }
    /// Get the requested implicit entry type, or Undefined when no entry was requested.
    [[nodiscard]] auto entryType() const -> vr::RuleType { return _entryType; }
    /// Set the optional title.
    void setTitle(text::String title) override { _title = std::move(title); }
    /// Set the optional description.
    void setDescription(text::String description) override { _description = std::move(description); }
    /// Set the custom validation error message.
    void setErrorMessage(text::String errorMessage) override { _errorMessage = std::move(errorMessage); }
    /// Set whether the target value is optional.
    void setOptional(bool isOptional) override { _isOptional = isOptional; }
    /// Set how text constraints compare characters.
    void setCaseSensitivity(text::CaseSensitivity caseSensitivity) override { _caseSensitivity = caseSensitivity; }
    /// Set whether the target value contains secret data.
    void setSecret(bool isSecret) override { _isSecret = isSecret; }
    /// Set the default value.
    void setDefaultValue(const conf::ValuePtr &value) override;
    /// Add a public builder constraint.
    void addConstraint(
        const vr::ConstraintPtr &constraint, text::String name, bool isNegated, text::String errorMessage) override;
    /// Add a public builder dependency.
    void addDependency(
        vr::DependencyMode mode,
        const std::vector<NamePathLike> &sources,
        const std::vector<NamePathLike> &targets,
        text::String errorMessage) override;
    /// Add a public builder key index.
    void addKeyIndex(
        const Name &name, const std::vector<NamePathLike> &keyPaths, text::CaseSensitivity caseSensitivity) override;
    /// Restrict this rule to a version list.
    void limitVersions(const std::vector<Integer> &versions, bool isNegated) override;
    /// Restrict this rule to a minimum version.
    void limitMinimumVersion(Integer version, bool isNegated) override;
    /// Restrict this rule to a maximum version.
    void limitMaximumVersion(Integer version, bool isNegated) override;
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
    /// Get the child that constrains this value's name, or an empty pointer.
    [[nodiscard]] auto nameConstraintRule() const -> RulePtr;
    /// Get the shared list-entry child, or an empty pointer.
    [[nodiscard]] auto listEntryRule() const -> RulePtr;
    /// Get the fallback child for unnamed matches, or an empty pointer.
    [[nodiscard]] auto anyValueRule() const -> RulePtr;
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
    /// Find a direct child by its exact definition name or alternative index.
    /// @param name The definition component; no escape decoding is performed.
    /// @return The child, or an empty pointer if it does not exist.
    [[nodiscard]] auto childByDefinitionName(const Name &name) const -> RulePtr;
    /// Follow an exact definition path relative to this rule, including alternative indexes.
    /// @param path The relative definition path; no escape decoding is performed.
    /// @return The descendant, or an empty pointer for an empty or unresolved path.
    [[nodiscard]] auto descendantByDefinitionPath(const NamePath &path) const -> RulePtr;
    /// Find a direct NamedValue child by its actual configuration name.
    /// @param name The value name; control roles and alternative branches are excluded.
    /// @return The named child, or an empty pointer; wildcard fallback is not performed.
    [[nodiscard]] auto childByValueName(const Name &name) const -> RulePtr;
    /// Follow a symbolic value path relative to this rule.
    /// Only vr_entry beneath a list or matrix selects a control rule. Escapes are never decoded.
    /// Alternatives can be returned, but traversal never selects one of their branches.
    /// @param path The relative path of regular names; document indexes and text names are unsupported.
    /// @return The descendant, or an empty pointer for an empty, unsupported or unresolved path.
    [[nodiscard]] auto descendantByValuePath(const NamePath &path) const -> RulePtr;
    /// Attach a child whose paths, role and parent have already been initialized.
    /// @param child The non-null rule; its definition name must be unique among these children.
    void addChild(const RulePtr &child) { _children.addRule(child); }

public: // construction helpers
    /// Classify a definition component before decoding any document escape.
    /// @param name An exact declaration name, or an index for an alternative branch.
    /// @return The matching control role, Alternative for an index, or NamedValue otherwise.
    [[nodiscard]] static auto roleForDefinitionName(const Name &name) -> RuleRole;

private:
    /// Find a direct child with the given role.
    [[nodiscard]] auto childByRole(RuleRole role) const -> RulePtr;
    /// Resolve one symbolic value-path component in the context of this rule.
    [[nodiscard]] auto childForValuePathComponent(const Name &name) const -> RulePtr;

public: // tests
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const Rule &rule) -> InternalViewPtr;
    friend auto internalView(const RulePtr &rule) -> InternalViewPtr;
#endif

private:
    RuleRole _role;               ///< The explicit purpose of this rule-tree node.
    Location _location;           ///< The source location of the declaration, including template sources.
    NamePath _definitionNamePath; ///< The expanded definition path, retaining escapes and indexes.
    NamePath _valueNamePath;      ///< The symbolic value path, retaining control placeholders.
    vr::RuleType _type;           ///< The type of the rule.
    vr::RuleType _entryType;      ///< Requested scalar entry type for the current builder call.
    std::vector<std::shared_ptr<const vr::builder::Attribute>> _entryAttributes; ///< Owned entry configuration.
    bool _hasEntryAttributes{false};                 ///< Whether the current call requested entry configuration.
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
