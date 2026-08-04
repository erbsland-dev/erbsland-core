// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InConstraint.hpp"

#include "InBytesConstraint.hpp"
#include "InFloatConstraint.hpp"
#include "InIntegerConstraint.hpp"
#include "InTextConstraint.hpp"
#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

#include <ranges>
#include <utility>

namespace erbsland::conf::impl {

using namespace text::literals;

auto handleInConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    const auto createConstraint = [&node, &rule]<typename Constraint, typename Value>() -> ConstraintPtr {
        auto values = typename Constraint::Values{node->asList<Value>()};
        if (std::ranges::empty(values)) {
            throwValidationError(
                text::StringFormat{"The '{}' constraint must specify a single {} value or a list of {} values"_el}
                    .build(node->name().asText(), rule->type().toText(), rule->type().toText()));
        }
        if (Constraint::hasDuplicate(values, rule->caseSensitivity())) {
            throwValidationError(
                text::StringFormat{"The '{}' list must not contain duplicate values"_el}.build(node->name().asText()));
        }
        return std::make_shared<Constraint>(std::move(values));
    };
    switch (context.rule->type().raw()) {
    case vr::RuleType::Integer:
        return createConstraint.template operator()<InIntegerConstraint, Integer>();
    case vr::RuleType::Float:
        return createConstraint.template operator()<InFloatConstraint, Float>();
    case vr::RuleType::Text:
        return createConstraint.template operator()<InTextConstraint, text::String>();
    case vr::RuleType::Bytes:
        return createConstraint.template operator()<InBytesConstraint, mem::ByteBlock>();
    default:
        break;
    }
    throwValidationError(
        text::StringFormat{"The '{}' constraint is not supported for '{}' rules"_el}.build(
            context.node->name().asText(), context.rule->type().toText()));
}

}
