// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MultipleConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate that integer values are multiples of a divisor.
class MultipleIntegerConstraint final : public MultipleConstraint<Integer> {
public:
    /// Create an integer multiple-of constraint.
    /// @param divisor The required divisor.
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
    /// Test if an integer violates the configured multiple constraint.
    [[nodiscard]] auto isNotValid(Integer tested) const -> bool;
};

}
