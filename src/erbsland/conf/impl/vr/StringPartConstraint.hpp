// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"
#include "ValidationContext.hpp"

#include "../../../text/StringList.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

class StringPartConstraint : public Constraint {
public:
    explicit StringPartConstraint(text::StringList values) : _expectedValues{std::move(values)} {}

protected: // implement Constraint
    void validateText(const ValidationContext &context, const text::String &value) const override;

protected: // interface for subclasses
    [[nodiscard]] virtual auto partText() const -> const text::String & = 0;
    [[nodiscard]] virtual auto doesPartMatch(
        const text::String &expectedValue, const text::String &testedValue, const ValidationContext &context) const
        -> bool = 0;

private:
    text::StringList _expectedValues;
};

class StartsConstraint final : public StringPartConstraint {
public:
    explicit StartsConstraint(text::StringList values) : StringPartConstraint(std::move(values)) {
        setType(vr::ConstraintType::Starts);
    }

protected:
    [[nodiscard]] auto partText() const -> const text::String & override {
        static const text::String text = "start with"_el;
        return text;
    }
    [[nodiscard]] auto doesPartMatch(
        const text::String &expectedValue, const text::String &testedValue, const ValidationContext &context) const
        -> bool override {

        return testedValue.startsWith(expectedValue, context.rule->caseSensitivity().asciiComparisonFn());
    }
};

class EndsConstraint final : public StringPartConstraint {
public:
    explicit EndsConstraint(text::StringList values) : StringPartConstraint(std::move(values)) {
        setType(vr::ConstraintType::Ends);
    }

protected:
    [[nodiscard]] auto partText() const -> const text::String & override {
        static const text::String text = "end with"_el;
        return text;
    }
    [[nodiscard]] auto doesPartMatch(
        const text::String &expectedValue, const text::String &testedValue, const ValidationContext &context) const
        -> bool override {

        return testedValue.endsWith(expectedValue, context.rule->caseSensitivity().asciiComparisonFn());
    }
};

class ContainsConstraint final : public StringPartConstraint {
public:
    explicit ContainsConstraint(text::StringList values) : StringPartConstraint(std::move(values)) {
        setType(vr::ConstraintType::Contains);
    }

protected:
    [[nodiscard]] auto partText() const -> const text::String & override {
        static const text::String text = "contain"_el;
        return text;
    }
    [[nodiscard]] auto doesPartMatch(
        const text::String &expectedValue, const text::String &testedValue, const ValidationContext &context) const
        -> bool override {

        return testedValue.contains(expectedValue, context.rule->caseSensitivity().asciiComparisonFn());
    }
};

auto handleStartsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;
auto handleEndsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;
auto handleContainsConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
