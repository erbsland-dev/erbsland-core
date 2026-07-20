// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

// Base template for common behavior (stores the divisor and provides helpers)
template <typename T>
class MultipleConstraint : public Constraint {
public:
    explicit MultipleConstraint(T divisor) : _divisor{divisor} { setType(vr::ConstraintType::Multiple); }

protected:
    [[nodiscard]] auto comparisonText() const -> const text::String & {
        static const text::String multipleOf = "must be a multiple of"_el;
        static const text::String notMultipleOf = "must not be a multiple of"_el;
        return isNegated() ? notMultipleOf : multipleOf;
    }

protected:
    T _divisor;
};

class MultipleIntegerConstraint final : public MultipleConstraint<Integer> {
public:
    explicit MultipleIntegerConstraint(Integer divisor);

protected:
    void validateInteger([[maybe_unused]] const ValidationContext &context, Integer value) const override;
    void validateText([[maybe_unused]] const ValidationContext &context, const text::String &value) const override;
    void validateBytes([[maybe_unused]] const ValidationContext &context, const mem::ByteBlock &value) const override;
    void validateValueList(const ValidationContext &context) const override;
    void validateSectionWithNames(const ValidationContext &context) const override;
    void validateSectionWithTexts(const ValidationContext &context) const override;
    void validateSectionList(const ValidationContext &context) const override;

private:
    [[nodiscard]] auto isNotValid(Integer tested) const -> bool;
};

class MultipleFloatConstraint final : public MultipleConstraint<Float> {
public:
    explicit MultipleFloatConstraint(Float divisor);

protected:
    void validateFloat([[maybe_unused]] const ValidationContext &context, Float value) const override;

private:
    [[nodiscard]] auto isNotValid(Float tested) const -> bool;
};

class MultipleMatrixConstraint final : public MultipleConstraint<Integer> {
public:
    explicit MultipleMatrixConstraint(Integer rowsDivisor, Integer columnsDivisor);

protected:
    void validateValueList(const ValidationContext &context) const override;

private:
    [[nodiscard]] auto isNotValidRows(Integer tested) const -> bool;
    [[nodiscard]] auto isNotValidColumns(Integer tested) const -> bool;

private:
    Integer _columnsDivisor;
};

// Factory
auto handleMultipleConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
