// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"

#include "../../../re/RegEx.hpp"

namespace erbsland::conf::impl {

class MatchesConstraint : public Constraint {
public:
    MatchesConstraint(const text::String &pattern, bool isVerbose);
    explicit MatchesConstraint(re::RegExPtr regex);
    ~MatchesConstraint() override = default;

protected:
    void validateText(const ValidationContext &context, const text::String &value) const override;

private:
    re::RegExPtr _regex; ///< The compiled regular expression.
};

auto handleMatchesConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
