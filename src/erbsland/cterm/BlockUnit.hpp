// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/IntegerUnit.hpp"

namespace erbsland::cterm {

/// The integer unit for terminal-block string positions and counts.
/// @tested{BlockUnitTest}
struct BlockUnit final : unit::IntegerUnit {};

}
