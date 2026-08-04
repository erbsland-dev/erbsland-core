// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InConstraint.hpp"

namespace erbsland::conf::impl {

/// Validate that text is one of the configured values.
class InTextConstraint final : public InConstraint<text::String> {
public:
    /// Creates a text membership constraint.
    /// @param values The allowed text values.
    explicit InTextConstraint(text::StringList values) : InConstraint(std::move(values)) {}

protected:
    void validateText(const ValidationContext &context, const text::String &value) const override;
};

}
