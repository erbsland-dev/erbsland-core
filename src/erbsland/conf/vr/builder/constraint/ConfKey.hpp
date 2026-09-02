// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../../../NamePath.hpp"

#include <utility>
#include <vector>

namespace erbsland::conf::vr::builder {

/// Adds a key-reference constraint to a named key index.
class ConfKey : public ConstraintAttribute {
public:
    /// Creates a key-reference constraint for one index path.
    /// @param reference The referenced index path.
    /// @param options Additional constraint options.
    explicit ConfKey(const NamePathLike &reference, ConstraintOptions options = {}) :
        _references{{reference}}, _options{std::move(options)} {}
    /// Creates a key-reference constraint for index paths.
    /// @param references The referenced index paths.
    /// @param options Additional constraint options.
    explicit ConfKey(std::vector<NamePathLike> references, ConstraintOptions options = {}) :
        _references{std::move(references)}, _options{std::move(options)} {}
    /// Creates a key-reference constraint for index paths.
    /// @param references The referenced index paths.
    /// @param options Additional constraint options.
    explicit ConfKey(const std::initializer_list<NamePathLike> references, ConstraintOptions options = {}) :
        _references{references}, _options{std::move(options)} {}

    void apply(RuleDefinition &rule) const override;

    std::vector<NamePathLike> _references;
    ConstraintOptions _options;
};

}
