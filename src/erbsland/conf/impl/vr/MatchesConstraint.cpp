// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "MatchesConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../re/RegExError.hpp"
#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

MatchesConstraint::MatchesConstraint(const text::String &pattern, const bool isVerbose) :
    Constraint{vr::ConstraintType::Matches} {
    try {
        auto flags = re::Flags{};
        if (isVerbose) {
            flags |= re::Flag::Verbose;
        }
        _regex = re::RegEx::compile(pattern, flags);
    } catch (const re::RegExError &error) {
        throwValidationError(text::StringFormat{"Invalid regular expression: {}"_el}.build(error.toString()));
    }
}

MatchesConstraint::MatchesConstraint(re::RegExPtr regex) :
    Constraint{vr::ConstraintType::Matches}, _regex{std::move(regex)} {
    if (_regex == nullptr) {
        throwValidationError("The regular expression in 'matches' constraint cannot be null"_el);
    }
}

void MatchesConstraint::validateText(const ValidationContext &, const text::String &value) const {
    if (_regex->findFirst(value) == nullptr) {
        throwValidationError("The text does not match an expected pattern"_el);
    }
}

auto handleMatchesConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    if (context.rule->type() != vr::RuleType::Text) {
        throwValidationError("The 'matches' constraint can only be used on text rules"_el);
    }
    if (context.node->type() != ValueType::RegEx) {
        throwValidationError("The 'matches' constraint requires a regular expression value"_el);
    }
    const auto regexValue = context.node->asRegEx();
    if (regexValue == nullptr || regexValue->pattern().isEmpty()) {
        throwValidationError("The regular expression in 'matches' constraint cannot be empty"_el);
    }
    return std::make_shared<MatchesConstraint>(regexValue);
}

}
