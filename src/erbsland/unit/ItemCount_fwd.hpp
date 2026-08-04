// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitAmount_fwd.hpp"
#include "ItemUnit.hpp"

namespace erbsland::unit {

/// A non-negative number of items.
using ItemCount = IntegerUnitAmount<ItemUnit>;

}
