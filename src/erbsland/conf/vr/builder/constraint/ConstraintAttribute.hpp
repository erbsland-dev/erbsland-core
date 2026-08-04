// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../attribute/Attribute.hpp"

#include "../../../vr/RuleType.hpp"

#include <initializer_list>

namespace erbsland::conf::vr::builder {

/// Base interface for builder constraints with common validation helpers.
class ConstraintAttribute : public Attribute {
protected:
    /// Require that a rule type is supported by this constraint.
    static void requireRuleTypeForConstraint(
        const impl::Rule &rule,
        const text::String &constraintName,
        const std::initializer_list<vr::RuleType> supportedTypes);

private:
    /// Test whether a rule has one of the supported types.
    [[nodiscard]] static auto hasRuleType(
        const impl::Rule &rule, const std::initializer_list<vr::RuleType> supportedTypes) -> bool;

    /// Throw an error for a constraint used with an unsupported rule.
    [[noreturn]] static void throwUnsupportedConstraint(const impl::Rule &rule, const text::String &constraintName);
};

}
