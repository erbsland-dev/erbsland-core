// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../text/String.hpp"
#include "../../../impl/vr/Constraint_fwd.hpp"
#include "../../../impl/vr/Rule_fwd.hpp"

namespace erbsland::conf::vr::builder {

/// Optional behavior shared by all builder constraints.
class ConstraintOptions {
public:
    bool isNegated{false};
    text::String errorMessage{};

    /// Return the constraint name including its negation prefix when configured.
    [[nodiscard]] auto prefixedConstraintName(const text::String &constraintName) const -> text::String;

    /// Apply these options to a constructed implementation constraint.
    void applyTo(impl::Constraint &constraint, const text::String &constraintName) const;

    /// Add a configured constraint to a rule.
    void addToRule(impl::Rule &rule, const impl::ConstraintPtr &constraint, const text::String &constraintName) const;

private:
    /// Return a name with an optional negation prefix.
    [[nodiscard]] static auto withNotPrefix(const text::String &name, const bool isNegated) -> text::String;
};

}
