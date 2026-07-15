// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColumnUnit.hpp"
#include "IntegerUnitIndex.hpp"

namespace erbsland::unit {

/// A zero-based source code column index.
using ColumnIndex = IntegerUnitIndex<ColumnUnit>;

}
