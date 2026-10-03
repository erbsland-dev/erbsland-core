// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitCount_fwd.hpp"
#include "U16Unit.hpp"

namespace erbsland::unit {

/// A non-negative count of unsigned 16-bit values.
using U16Count = IntegerUnitCount<U16Unit>;

}
