// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitRange.hpp"
#include "U16DataUnit.hpp"

namespace erbsland::unit {

/// A range for UTF-16 data.
using U16DataRange = IntegerUnitRange<U16DataUnit>;

}
