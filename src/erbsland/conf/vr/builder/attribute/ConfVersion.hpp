// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../Integer.hpp"

#include <vector>

namespace erbsland::conf::vr::builder {

/// Restricts a rule to specific versions.
class ConfVersion : public Attribute {
public:
    /// Creates a configuration-version attribute from a version list.
    /// @param versions The accepted configuration versions.
    /// @param isNegated `true` to reject the listed versions.
    explicit ConfVersion(std::vector<Integer> versions, const bool isNegated = false) :
        _versions{std::move(versions)}, _isNegated{isNegated} {}
    /// Creates a configuration-version attribute from a version list.
    /// @param versions The accepted configuration versions.
    /// @param isNegated `true` to reject the listed versions.
    explicit ConfVersion(const std::initializer_list<Integer> versions, const bool isNegated = false) :
        _versions{versions}, _isNegated{isNegated} {}
    /// Creates a configuration-version attribute from one version.
    /// @param version The accepted configuration version.
    /// @param isNegated `true` to reject the version.
    explicit ConfVersion(const Integer version, const bool isNegated = false) :
        _versions{version}, _isNegated{isNegated} {}

    void apply(RuleDefinition &rule) const override;

    std::vector<Integer> _versions;
    bool _isNegated{false};
};

}
