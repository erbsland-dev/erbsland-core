// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CpUnit.hpp"
#include "IntegerUnitCount_fwd.hpp"

namespace erbsland::unit {

/// A non-negative count of Unicode code points.
using CpCount = IntegerUnitCount<CpUnit>;

}
