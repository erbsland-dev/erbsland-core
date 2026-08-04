// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EqualsConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate equality of text values using rule case-sensitivity settings.
class EqualsTextConstraint final : public EqualsConstraint<text::String> {
public:
    /// Creates a text equality constraint from its expected value.
    /// @tparam Fwd A forwarding reference to the expected text type.
    /// @param expected The expected text.
    template <typename Fwd>
        requires(std::is_same_v<std::remove_cvref_t<Fwd>, text::String>)
    explicit EqualsTextConstraint(Fwd &&expected) : EqualsConstraint(std::forward<Fwd>(expected)) {}

protected:
    void validateText(const ValidationContext &context, const text::String &value) const override;
};

}
