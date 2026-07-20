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
struct ConfKey : ConstraintAttribute {
    explicit ConfKey(const NamePathLike &reference, ConstraintOptions options = {}) :
        _references{{reference}}, _options{std::move(options)} {}
    explicit ConfKey(std::vector<NamePathLike> references, ConstraintOptions options = {}) :
        _references{std::move(references)}, _options{std::move(options)} {}
    explicit ConfKey(const std::initializer_list<NamePathLike> references, ConstraintOptions options = {}) :
        _references{references}, _options{std::move(options)} {}

    void operator()(impl::Rule &rule) override;

    std::vector<NamePathLike> _references;
    ConstraintOptions _options;
};

}
