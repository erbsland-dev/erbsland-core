// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ElementUnit.hpp"
#include "IntegerUnitOffset.hpp"

namespace erbsland::unit {

/// A signed offset in element units.
using ElementOffset = IntegerUnitOffset<ElementUnit>;

}
