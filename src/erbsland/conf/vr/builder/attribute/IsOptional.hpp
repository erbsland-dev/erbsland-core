// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

namespace erbsland::conf::vr::builder {

/// Marks a rule as optional or required.
class IsOptional : public Attribute {
public:
    /// Set whether a rule is optional.
    /// @param isOptional `true` to mark the rule optional.
    explicit IsOptional(const bool isOptional = true) : _isOptional{isOptional} {}
    void apply(RuleDefinition &rule) const override;
    bool _isOptional{true};
};

}
