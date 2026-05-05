// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitOffset.hpp"
#include "U16DataUnit.hpp"

namespace erbsland::unit {

/// An offset for UTF-16 data.
using U16DataOffset = IntegerUnitOffset<U16DataUnit>;

}
