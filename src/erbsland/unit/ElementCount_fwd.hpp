// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ElementUnit.hpp"
#include "IntegerUnitAmount_fwd.hpp"

namespace erbsland::unit {

/// A non-negative number of elements.
using ElementCount = IntegerUnitAmount<ElementUnit>;

}
