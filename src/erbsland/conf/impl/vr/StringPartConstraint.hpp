// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"
#include "ValidationContext.hpp"

#include "../../../text/StringList.hpp"

#include <utility>

namespace erbsland::conf::impl {

using namespace text::literals;

/// Provides shared validation for string prefix, suffix, and containment constraints.
class StringPartConstraint : public Constraint {
public:
    /// Creates a string-part constraint.
    /// @param type The concrete string-part constraint kind.
    /// @param values The expected string parts.
    StringPartConstraint(vr::ConstraintType type, text::StringList values) :
        Constraint{type}, _expectedValues{std::move(values)} {}

    /// Parse and validate string-part constraint values.
    /// @param context The constraint handler context.
    /// @return The configured values.
    [[nodiscard]] static auto valuesFromContext(const ConstraintHandlerContext &context) -> text::StringList;

protected: // implement Constraint
    void validateText(const ValidationContext &context, const text::String &value) const override;

protected: // interface for subclasses
    /// Get the text used to describe the tested string part.
    [[nodiscard]] virtual auto partText() const -> const text::String & = 0;
    /// Test whether a candidate string contains the expected part.
    [[nodiscard]] virtual auto doesPartMatch(
        const text::String &expectedValue, const text::String &testedValue, const ValidationContext &context) const
        -> bool = 0;

private:
    text::StringList _expectedValues;
};

/// Create a string-prefix constraint from handler input.
auto handleStartsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;
/// Create a string-suffix constraint from handler input.
auto handleEndsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;
/// Create a string-containment constraint from handler input.
auto handleContainsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
