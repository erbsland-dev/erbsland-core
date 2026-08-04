// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EqualsConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate equality of numeric values and collection sizes.
class EqualsIntegerConstraint final : public EqualsConstraint<Integer> {
public:
    /// Creates an integer equality constraint.
    /// @param value The expected integer value.
    explicit EqualsIntegerConstraint(Integer value);

protected:
    void validateInteger(const ValidationContext &context, Integer value) const override;
    void validateText(const ValidationContext &context, const text::String &value) const override;
    void validateBytes(const ValidationContext &context, const mem::ByteBlock &value) const override;
    void validateValueList(const ValidationContext &context) const override;
    void validateSectionWithNames(const ValidationContext &context) const override;
    void validateSectionWithTexts(const ValidationContext &context) const override;
    void validateSectionList(const ValidationContext &context) const override;
};

}
