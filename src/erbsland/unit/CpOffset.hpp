// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CpUnit.hpp"
#include "IntegerUnitOffset.hpp"

namespace erbsland::unit {

/// A Unicode code-point-based offset.
using CpOffset = IntegerUnitOffset<CpUnit>;

}
