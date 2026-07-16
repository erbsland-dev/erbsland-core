// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CpUnit.hpp"
#include "IntegerUnitRange_fwd.hpp"

namespace erbsland::unit {

/// A Unicode code-point-based range.
using CpRange = IntegerUnitRange<CpUnit>;

}
