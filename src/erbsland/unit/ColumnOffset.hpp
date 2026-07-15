// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColumnUnit.hpp"
#include "IntegerUnitOffset.hpp"

namespace erbsland::unit {

/// A signed source code column offset.
using ColumnOffset = IntegerUnitOffset<ColumnUnit>;

}
