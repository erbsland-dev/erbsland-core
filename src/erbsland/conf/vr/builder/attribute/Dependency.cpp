// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Dependency.hpp"

namespace erbsland::conf::vr::builder {

void Dependency::apply(RuleDefinition &rule) const {
    rule.addDependency(_mode, _sources, _targets, _errorMessage);
}

}
