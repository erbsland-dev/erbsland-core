// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>
#include <vector>

namespace erbsland::conf::impl {

class DependencyDefinition;
using DependencyDefinitionPtr = std::shared_ptr<DependencyDefinition>;
using DependencyDefinitionList = std::vector<DependencyDefinitionPtr>;

}
