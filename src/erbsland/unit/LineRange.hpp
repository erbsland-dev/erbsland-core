// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitRange.hpp"
#include "LineUnit.hpp"

namespace erbsland::unit {

/// A range of source code lines.
using LineRange = IntegerUnitRange<LineUnit>;

}
