// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitOffset.hpp"
#include "ItemUnit.hpp"

namespace erbsland::unit {

/// A signed offset in item units.
using ItemOffset = IntegerUnitOffset<ItemUnit>;

}
