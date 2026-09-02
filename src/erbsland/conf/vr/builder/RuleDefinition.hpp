// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Constraint_fwd.hpp"
#include "../DependencyMode.hpp"
#include "../RuleType.hpp"

#include "../../../text/CaseSensitivity.hpp"
#include "../../../text/String.hpp"
#include "../../Integer.hpp"
#include "../../NamePath.hpp"
#include "../../Value_fwd.hpp"

#include <vector>

namespace erbsland::conf::vr::builder {

/// Stable mutation interface used while a validation rule is being built.
/// Instances are owned by `RulesBuilder`; attributes may only modify the rule passed to `Attribute::apply()`.
/// @tested{VrBuilderApiTest VrManualConstructionTest}
class RuleDefinition {
public:
    // defaults
    virtual ~RuleDefinition() = default;

public: // properties
    /// Get the configured rule type.
    [[nodiscard]] virtual auto type() const -> RuleType = 0;
    /// Get the configured case sensitivity.
    [[nodiscard]] virtual auto caseSensitivity() const -> text::CaseSensitivity = 0;
    /// Set the configured rule type.
    /// @param type The new rule type.
    virtual void setType(RuleType type) = 0;
    /// Set the user-facing title.
    /// @param title The new title.
    virtual void setTitle(text::String title) = 0;
    /// Set the descriptive text.
    /// @param description The new description.
    virtual void setDescription(text::String description) = 0;
    /// Set the custom validation error message.
    /// @param errorMessage The new error message.
    virtual void setErrorMessage(text::String errorMessage) = 0;
    /// Set whether the validated value is optional.
    /// @param isOptional `true` if the value may be omitted.
    virtual void setOptional(bool isOptional) = 0;
    /// Set how text constraints compare characters.
    /// @param caseSensitivity The comparison mode.
    virtual void setCaseSensitivity(text::CaseSensitivity caseSensitivity) = 0;
    /// Set whether the validated value contains secret data.
    /// @param isSecret `true` if diagnostics must protect the value.
    virtual void setSecret(bool isSecret) = 0;
    /// Set the default value.
    /// @param value The default configuration value.
    virtual void setDefaultValue(const ValuePtr &value) = 0;

public: // constraints and relations
    /// Add a constraint, replacing an existing constraint of the same type.
    /// @param constraint The constraint implementation created by a builder attribute.
    /// @param name The public constraint name without a negation prefix.
    /// @param isNegated Whether to negate the constraint.
    /// @param errorMessage An optional custom validation error.
    virtual void addConstraint(
        const ConstraintPtr &constraint, text::String name, bool isNegated, text::String errorMessage) = 0;
    /// Add a dependency relation to this rule.
    /// @param mode The relation between sources and targets.
    /// @param sources The source paths relative to this rule.
    /// @param targets The target paths relative to this rule.
    /// @param errorMessage An optional custom validation error.
    virtual void addDependency(
        DependencyMode mode,
        const std::vector<NamePathLike> &sources,
        const std::vector<NamePathLike> &targets,
        text::String errorMessage) = 0;
    /// Add a key index to this rule.
    /// @param name The optional index name.
    /// @param keyPaths The indexed paths relative to this rule.
    /// @param caseSensitivity The key comparison mode.
    virtual void addKeyIndex(
        const Name &name, const std::vector<NamePathLike> &keyPaths, text::CaseSensitivity caseSensitivity) = 0;

public: // versions
    /// Restrict this rule to the listed versions or their complement.
    /// @param versions The configuration versions to select.
    /// @param isNegated `true` to select the complement.
    virtual void limitVersions(const std::vector<Integer> &versions, bool isNegated) = 0;
    /// Restrict this rule to versions at or above the boundary, or its complement.
    /// @param version The inclusive lower boundary.
    /// @param isNegated `true` to select versions below the boundary.
    virtual void limitMinimumVersion(Integer version, bool isNegated) = 0;
    /// Restrict this rule to versions at or below the boundary, or its complement.
    /// @param version The inclusive upper boundary.
    /// @param isNegated `true` to select versions above the boundary.
    virtual void limitMaximumVersion(Integer version, bool isNegated) = 0;
};

}
