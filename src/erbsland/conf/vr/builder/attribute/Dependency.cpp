// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Dependency.hpp"

#include "../../../impl/vr/DependencyDefinition.hpp"
#include "../../../impl/vr/NamePathHelper.hpp"
#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

void Dependency::operator()(Rule &rule) {
    auto sourcePaths = impl::parseNamePathList(_sources);
    auto targetPaths = impl::parseNamePathList(_targets);
    auto definition =
        impl::DependencyDefinition::create(_mode, std::move(sourcePaths), std::move(targetPaths), _errorMessage);
    rule.addDependencyDefinition(definition);
}

}
