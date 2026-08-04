// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "Rule_fwd.hpp"
#include "RuleType.hpp"

#include "../NamePath.hpp"

#include "../../text/CaseSensitivity.hpp"
#include "../../text/String.hpp"

namespace erbsland::conf::vr {

/// Base interface for a validation rule.
class Rule {
public:
    // defaults
    virtual ~Rule() = default;

public:
    /// The target name-path of this rule.
    /// This is the name path of the value in the validated document.
    [[nodiscard]] virtual auto namePath() const -> NamePath = 0;
    /// The type of the rule.
    [[nodiscard]] virtual auto type() const -> RuleType = 0;
    /// An optional title.
    [[nodiscard]] virtual auto title() const -> text::String = 0;
    /// An optional description.
    [[nodiscard]] virtual auto description() const -> text::String = 0;
    /// If this rule has a default.
    [[nodiscard]] virtual auto hasDefault() const -> bool = 0;
    /// Test if this rule has a custom error message.
    [[nodiscard]] virtual auto hasCustomError() const -> bool = 0;
    /// A custom error message for the rule.
    [[nodiscard]] virtual auto customError() const -> text::String = 0;
    /// A list of constraints.
    [[nodiscard]] virtual auto constraints() const -> std::vector<ConstraintPtr> = 0;
    /// If this rule is optional.
    [[nodiscard]] virtual auto isOptional() const -> bool = 0;
    /// If constraints are tested case-sensitive.
    [[nodiscard]] virtual auto caseSensitivity() const -> text::CaseSensitivity = 0;
    /// If the validated value is a secret.
    [[nodiscard]] virtual auto isSecret() const -> bool = 0;
    /// A list of child rules.
    [[nodiscard]] virtual auto children() const -> std::vector<RulePtr> = 0;

public: // location
    /// Test if there is location info.
    [[nodiscard]] virtual auto hasLocation() const noexcept -> bool = 0;
    /// Get the location info.
    [[nodiscard]] virtual auto location() const noexcept -> const Location & = 0;
    /// Set the location info.
    virtual void setLocation(const Location &newLocation) noexcept = 0;
};

}
