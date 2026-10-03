// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteUnit.hpp"
#include "IntegerUnitCount_fwd.hpp"

namespace erbsland::unit {

/// A non-negative count of bytes.
using ByteCount = IntegerUnitCount<ByteUnit>;

}
