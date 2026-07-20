// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "DependencyDefinition.hpp"
#include "KeyDefinition.hpp"
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

class Rule;
using RulePtr = std::shared_ptr<Rule>;
using RuleWeakPtr = std::weak_ptr<Rule>;

class Rule : public vr::Rule {
public:
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
    [[nodiscard]] auto ruleNamePath() const -> const NamePath & { return _ruleNamePath; }
    [[nodiscard]] auto ruleName() const -> const Name & {
        return _ruleNamePath.empty() ? vrc::cEmptyName : _ruleNamePath.back();
    }
    [[nodiscard]] auto targetNamePath() const -> const NamePath & { return _targetNamePath; }
    [[nodiscard]] auto targetName() const -> const Name & {
        return _targetNamePath.empty() ? vrc::cEmptyName : _targetNamePath.back();
    }
    [[nodiscard]] auto defaultValue() const -> const ValuePtr & { return _defaultValue; }
    [[nodiscard]] auto childrenImpl() const -> const RuleMap & { return _children; }
    [[nodiscard]] auto parent() const -> RulePtr { return _parent.lock(); }
    void setRuleNamePath(const NamePath &namePath) { _ruleNamePath = namePath; }
    void setTargetNamePath(const NamePath &namePath) { _targetNamePath = namePath; }
    void setType(const vr::RuleType type) { _type = type; }
    void setTitle(text::String &&title) noexcept { _title = std::move(title); }
    void setTitle(const text::String &title) noexcept { _title = title; }
    void setDescription(text::String &&description) noexcept { _description = std::move(description); }
    void setDescription(const text::String &description) noexcept { _description = description; }
    void setErrorMessage(text::String &&errorMessage) noexcept { _errorMessage = std::move(errorMessage); }
    void setErrorMessage(const text::String &errorMessage) noexcept { _errorMessage = errorMessage; }
    void setOptional(bool isOptional) { _isOptional = isOptional; }
    void setCaseSensitivity(const text::CaseSensitivity caseSensitivity) { _caseSensitivity = caseSensitivity; }
    void setSecret(bool isSecret) { _isSecret = isSecret; }
    void setDefaultValue(const ValuePtr &value) { _defaultValue = value; }
    void addOrOverwriteConstraint(const ConstraintPtr &constraint);
    [[nodiscard]] auto hasConstraint(vr::ConstraintType type) const -> bool;
    [[nodiscard]] auto hasConstraint(const text::String &name) const -> bool;
    [[nodiscard]] auto constraint(const text::String &name) const -> ConstraintPtr;
    [[nodiscard]] auto constraint(vr::ConstraintType type) const -> ConstraintPtr;
    [[nodiscard]] auto constraintsImpl() const -> const ConstraintList & { return _constraints; }
    [[nodiscard]] auto hasNameConstraints() const -> bool { return _children.hasRule(vrc::cReservedName); }
    [[nodiscard]] auto nameConstraints() const -> RulePtr { return _children.rule(vrc::cReservedName); }
    void addKeyDefinition(const KeyDefinitionPtr &keyDefinition);
    [[nodiscard]] auto hasKeyDefinitions() const -> bool;
    [[nodiscard]] auto keyDefinitions() const -> const KeyDefinitionList &;
    [[nodiscard]] auto hasDependencyDefinitions() const -> bool { return !_dependencyDefinitions.empty(); }
    [[nodiscard]] auto dependencyDefinitions() const -> const DependencyDefinitionList & {
        return _dependencyDefinitions;
    }
    void addDependencyDefinition(const DependencyDefinitionPtr &dependencyDefinition);
    void limitVersionMask(const VersionMask &versionMask) { _versionMask &= versionMask; }
    [[nodiscard]] auto versionMask() const -> const VersionMask & { return _versionMask; }
    void setParent(const RulePtr &parent) { _parent = RuleWeakPtr{parent}; }
    [[nodiscard]] auto empty() const -> bool { return _children.empty(); }
    [[nodiscard]] auto hasChild(const Name &name) const -> bool { return _children.hasRule(name); }
    [[nodiscard]] auto child(const Name &name) const -> RulePtr;
    [[nodiscard]] auto child(const NamePath &namePath) const -> RulePtr;
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
