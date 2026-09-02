// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../Integer.hpp"

namespace erbsland::conf::vr::builder {

/// Restricts a rule to versions greater than or equal to a minimum.
class MinimumVersion : public Attribute {
public:
    /// Set the minimum permitted version.
    /// @param version The version bound.
    /// @param isNegated Whether to negate the version condition.
    explicit MinimumVersion(const Integer version, const bool isNegated = false) :
        _version{version}, _isNegated{isNegated} {}
    void apply(RuleDefinition &rule) const override;
    Integer _version;
    bool _isNegated{false};
};

}
