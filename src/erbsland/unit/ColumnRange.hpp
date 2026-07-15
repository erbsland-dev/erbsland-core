// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColumnUnit.hpp"
#include "IntegerUnitRange.hpp"

namespace erbsland::unit {

/// A range of source code columns.
using ColumnRange = IntegerUnitRange<ColumnUnit>;

}
