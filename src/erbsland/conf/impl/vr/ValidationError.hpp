// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule_fwd.hpp"

#include "../value/Value_fwd.hpp"

#include "../../ConfError.hpp"
#include "../../Integer.hpp"
#include "../../vr/RuleType.hpp"

#include <set>

namespace erbsland::conf::impl {

/// Throw a validation error.
template <typename... Args>
[[noreturn]] void throwValidationError(text::String message, Args &&...args) {
    throw ConfError(ConfErrorCategory::Validation, std::move(message), std::forward<Args>(args)...);
}

/// Create a text with possible types.
[[nodiscard]] auto expectedRuleTypesText(const std::vector<vr::RuleType> &ruleTypes) -> text::String;

/// Create an error message string with the expected value types from a rule.
/// @param rule The rule.
/// @param version The version of the document.
/// @return The textual representation of the expected value type.
[[nodiscard]] auto expectedValueTypeText(const RulePtr &rule, Integer version) -> text::String;

/// Throw an error message when we got a value of an unexpected type.
/// @param rule The rule.
/// @param value The value with the unexpected type.
/// @param version The version of the document.
[[noreturn]] void throwExpectedVsActual(const RulePtr &rule, const ValuePtr &value, Integer version);

}
