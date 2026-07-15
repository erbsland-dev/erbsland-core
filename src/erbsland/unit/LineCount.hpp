// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitAmount.hpp"
#include "LineUnit.hpp"

namespace erbsland::unit {

/// A source code line count.
using LineCount = IntegerUnitAmount<LineUnit>;

}
