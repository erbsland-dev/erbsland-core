// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/Character.hpp"

#include <cstdint>
#include <vector>

namespace erbsland::re::impl {

using SequenceData = std::vector<text::Char>;
using SequenceIndex = uint16_t;
using SequenceLength = uint16_t;

}
