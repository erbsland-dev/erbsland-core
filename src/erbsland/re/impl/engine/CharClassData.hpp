// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/CharClass.hpp"

#include <cstdint>
#include <vector>

namespace erbsland::re::impl {

using CharClassData = std::vector<CharClass>;
using CharClassIndex = uint16_t;

}
