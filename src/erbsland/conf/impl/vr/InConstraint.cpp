// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InConstraint.hpp"

#include "ValidationError.hpp"

#include "../../../text/StringFormat.hpp"

#include <ranges>

namespace erbsland::conf::impl {

using namespace text::literals;

void InIntegerConstraint::validateInteger(const ValidationContext &context, const Integer value) const {
    if (isNotValid(value, context)) {
        text::StringEditor expected;
        for (std::size_t i = 0; i < _values.size(); ++i) {
            if (i != 0) {
                expected.append(" or "_el);
            }
            expected.append(text::StringFormat{"{}"_el}.build(_values[i]));
        }
        throwValidationError(text::StringFormat{"The value {} {}"_el}.build(comparisonText(), expected));
    }
}

void InFloatConstraint::validateFloat(const ValidationContext &context, const Float value) const {
    if (isNotValid(value, context)) {
        text::StringEditor expected;
        for (std::size_t i = 0; i < _values.size(); ++i) {
            if (i != 0) {
                expected.append(" or "_el);
            }
            expected.append(text::StringFormat{"{:.6}"_el}.build(_values[i]));
        }
        throwValidationError(
            text::StringFormat{"The value {} {} (within platform tolerance)"_el}.build(comparisonText(), expected));
    }
}

void InTextConstraint::validateText(const ValidationContext &context, const text::String &value) const {
    if (isNotValid(value, context)) {
        text::StringEditor expected;
        auto isFirst = true;
        for (const auto &expectedValue : _values) {
            if (!isFirst) {
                expected.append(" or "_el);
            }
            isFirst = false;
            expected.append(text::StringFormat{"\"{:/display}\""_el}.build(expectedValue));
        }
        throwValidationError(
            text::StringFormat{"The text {} {} ({})"_el}.build(
                comparisonText(), expected, context.rule->caseSensitivity().toString()));
    }
}

void InBytesConstraint::validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const {
    if (isNotValid(value, context)) {
        text::StringEditor expected;
        for (std::size_t i = 0; i < _values.size(); ++i) {
            if (i != 0) {
                expected.append(" or "_el);
            }
            expected.append(text::StringFormat{"\"{:bytes:maximum=16,truncate=middle}\""_el}.build(_values[i]));
        }
        throwValidationError(text::StringFormat{"The byte sequence {} {}"_el}.build(comparisonText(), expected));
    }
}

namespace {
template <typename Constraint, typename ValueType>
[[nodiscard]] auto createInConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    const auto &node = context.node;
    const auto &rule = context.rule;
    const auto values = typename Constraint::Values{node->asList<ValueType>()};
    if (std::ranges::empty(values)) {
        throwValidationError(
            text::StringFormat{"The '{}' constraint must specify a single {} value or a list of {} values"_el}.build(
                node->name().asText(), rule->type().toText(), rule->type().toText()));
    }
    if (Constraint::hasDuplicate(values, rule->caseSensitivity())) {
        throwValidationError(
            text::StringFormat{"The '{}' list must not contain duplicate values"_el}.build(node->name().asText()));
    }
    return std::make_shared<Constraint>(values);
}
}

auto handleInConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr {
    switch (context.rule->type().raw()) {
    case vr::RuleType::Integer:
        return createInConstraint<InIntegerConstraint, Integer>(context);
    case vr::RuleType::Float:
        return createInConstraint<InFloatConstraint, Float>(context);
    case vr::RuleType::Text:
        return createInConstraint<InTextConstraint, text::String>(context);
    case vr::RuleType::Bytes:
        return createInConstraint<InBytesConstraint, mem::ByteBlock>(context);
    default:
        break;
    }
    throwValidationError(
        text::StringFormat{"The '{}' constraint is not supported for '{}' rules"_el}.build(
            context.node->name().asText(), context.rule->type().toText()));
}

}
