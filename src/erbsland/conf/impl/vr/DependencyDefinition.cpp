// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DependencyDefinition.hpp"

namespace erbsland::conf::impl {

DependencyDefinition::DependencyDefinition(
    const DependencyMode mode, NamePathList sources, NamePathList targets, text::String errorMessage) noexcept :
    _mode(mode), _sources(std::move(sources)), _targets(std::move(targets)), _errorMessage(std::move(errorMessage)) {
}

auto DependencyDefinition::create(
    const DependencyMode mode, NamePathList sources, NamePathList targets, text::String errorMessage)
    -> DependencyDefinitionPtr {

    return std::make_shared<DependencyDefinition>(
        mode, std::move(sources), std::move(targets), std::move(errorMessage));
}

}
