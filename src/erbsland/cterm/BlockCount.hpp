// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockUnit.hpp"

#include "../unit/IntegerUnitAmount.hpp"

namespace erbsland::cterm {

/// A block-string count or length.
/// @tested{BlockUnitTest}
using BlockCount = unit::IntegerUnitAmount<BlockUnit>;

}
