// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CpUnit.hpp"
#include "IntegerUnitAmount.hpp"

namespace erbsland::unit {

/// A Unicode code-point-based length.
using CpLength = IntegerUnitAmount<CpUnit>;

}
