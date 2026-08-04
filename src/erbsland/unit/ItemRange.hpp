// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitRange.hpp"
#include "ItemUnit.hpp"

namespace erbsland::unit {

/// A half-open range of items.
using ItemRange = IntegerUnitRange<ItemUnit>;

}
