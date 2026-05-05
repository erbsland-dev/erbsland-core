// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ElementUnit.hpp"
#include "IntegerUnitRange.hpp"

namespace erbsland::unit {

/// A half-open range of elements.
using ElementRange = IntegerUnitRange<ElementUnit>;

}
