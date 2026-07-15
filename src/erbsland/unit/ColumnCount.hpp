// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ColumnUnit.hpp"
#include "IntegerUnitAmount.hpp"

namespace erbsland::unit {

/// A source code column count.
using ColumnCount = IntegerUnitAmount<ColumnUnit>;

}
