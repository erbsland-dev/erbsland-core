// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>
#include <vector>

namespace erbsland::conf::impl {

class KeyIndex;
using KeyIndexPtr = std::shared_ptr<KeyIndex>;
using KeyIndexList = std::vector<KeyIndexPtr>;

}
