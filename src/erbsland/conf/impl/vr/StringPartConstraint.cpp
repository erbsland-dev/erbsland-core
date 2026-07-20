// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringPartConstraint.hpp"

#include "MinMaxConstraint.hpp"
#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

void StringPartConstraint::validateText(const ValidationContext &context, const text::String &value) const {
    bool doesMatch = false;
    for (const auto &expectedValue : _expectedValues) {
        if (doesPartMatch(expectedValue, value, context)) {
            doesMatch = true;
            break;
        }
    }
    if (doesMatch == isNegated()) {
        text::StringEditor expected;
        for (const auto &expValue : _expectedValues) {
            if (!expected.isEmpty()) {
                expected.append(" or "_el);
            }
            const auto escapedValue = text::StringFormat{"{:/display}"_el}.build(expValue).truncated(
                unit::CpLength::fromSizeT(200), text::TruncateMode::Middle, "…"_el);
            expected.append(text::StringFormat{"\"{}\""_el}.build(escapedValue));
        }
        throwValidationError(
            text::StringFormat{"The text {} {} {} ({})"_el}.build(
                text::String{isNegated() ? "must not"_el : "does not"_el},
                partText(),
                text::String{expected},
                context.rule->caseSensitivity().toString()));
    }
}

namespace {
template <typename Constraint>
[[nodiscard]] auto createConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    if (rule->type() == vr::RuleType::Text) {
        const auto textValues = node->asList<text::String>();
        if (textValues.empty()) {
            throwValidationError(
                text::StringFormat{"The '{}' constraint must specify a single text value or a list of texts"_el}.build(
                    node->name().asText()));
        }
        return std::make_shared<Constraint>(text::StringList{textValues});
    }
    throwValidationError(
        text::StringFormat{"The '{}' constraint is not supported for '{}' rules"_el}.build(
            node->name().asText(), rule->type().toText()));
}
}

auto handleStartsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    return createConstraint<StartsConstraint>(context);
}

auto handleEndsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    return createConstraint<EndsConstraint>(context);
}

auto handleContainsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    return createConstraint<ContainsConstraint>(context);
}

}
