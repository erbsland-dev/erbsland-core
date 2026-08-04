// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TypedMinMaxConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate integer and collection minimum or maximum limits.
/// @tested{VrMinimumTest VrMaximumTest VrBuilderApiTest}
class MinMaxIntegerConstraint final : public TypedMinMaxConstraint<Integer> {
public:
    /// Create an integer minimum or maximum constraint.
    explicit MinMaxIntegerConstraint(const MinOrMax minOrMax, const Integer value) :
        TypedMinMaxConstraint{minOrMax, value} {}

protected:
    void validateInteger(const ValidationContext &context, Integer value) const override;
    void validateText(const ValidationContext &context, const text::String &value) const override;
    void validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const override;
    void validateValueList(const ValidationContext &context) const override;
    void validateSectionList(const ValidationContext &context) const override;
    void validateSectionWithNames(const ValidationContext &context) const override;
    void validateSectionWithTexts(const ValidationContext &context) const override;
};

}
