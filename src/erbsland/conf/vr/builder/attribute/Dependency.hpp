// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../impl/vr/DependencyMode.hpp"
#include "../../../NamePath.hpp"

#include <utility>
#include <vector>

namespace erbsland::conf::vr::builder {

/// Adds a dependency relation between source and target paths.
class Dependency : public Attribute {
public:
    /// Creates a dependency relation from path collections.
    /// @param mode The dependency mode.
    /// @param sources The source paths.
    /// @param targets The target paths.
    /// @param errorMessage The optional validation error message.
    Dependency(
        const impl::DependencyMode mode,
        std::vector<NamePathLike> sources,
        std::vector<NamePathLike> targets,
        text::String errorMessage = {}) :
        _mode{mode},
        _sources{std::move(sources)},
        _targets{std::move(targets)},
        _errorMessage{std::move(errorMessage)} {}

    /// Creates a dependency relation from path lists.
    /// @param mode The dependency mode.
    /// @param sources The source paths.
    /// @param targets The target paths.
    /// @param errorMessage The optional validation error message.
    Dependency(
        const impl::DependencyMode mode,
        const std::initializer_list<NamePathLike> sources,
        const std::initializer_list<NamePathLike> targets,
        text::String errorMessage = {}) :
        Dependency(
            mode, std::vector<NamePathLike>{sources}, std::vector<NamePathLike>{targets}, std::move(errorMessage)) {}

    void operator()(Rule &rule) override;

    impl::DependencyMode _mode{impl::DependencyMode::Undefined};
    std::vector<NamePathLike> _sources;
    std::vector<NamePathLike> _targets;
    text::String _errorMessage;
};

}
