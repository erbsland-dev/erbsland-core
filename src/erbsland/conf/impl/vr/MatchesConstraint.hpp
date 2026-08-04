// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"

#include "../../../re/RegEx.hpp"

namespace erbsland::conf::impl {

/// Constraint that validates text against a regular expression.
class MatchesConstraint : public Constraint {
public:
    /// Creates a regular-expression constraint from a pattern.
    /// @param pattern The regular-expression pattern.
    /// @param isVerbose `true` to enable verbose pattern syntax.
    MatchesConstraint(const text::String &pattern, bool isVerbose);
    /// Creates a regular-expression constraint from a compiled expression.
    /// @param regex The compiled regular expression.
    explicit MatchesConstraint(re::RegExPtr regex);

    // defaults
    ~MatchesConstraint() override = default;

protected:
    /// Validate one text value against the compiled expression.
    void validateText(const ValidationContext &context, const text::String &value) const override;

private:
    re::RegExPtr _regex; ///< The compiled regular expression.
};

/// Create a regular-expression matching constraint from its parsed definition.
auto handleMatchesConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
