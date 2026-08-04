// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>
#include <vector>

namespace erbsland::conf::impl {

class KeyDefinition;
using KeyDefinitionPtr = std::shared_ptr<KeyDefinition>;
using KeyDefinitionList = std::vector<KeyDefinitionPtr>;

}
