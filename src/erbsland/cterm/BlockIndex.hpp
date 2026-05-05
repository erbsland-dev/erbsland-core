// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockUnit.hpp"

#include "../unit/IntegerUnitIndex.hpp"

namespace erbsland::cterm {

/// A block-string index.
/// @tested{BlockUnitTest}
using BlockIndex = unit::IntegerUnitIndex<BlockUnit>;

}
