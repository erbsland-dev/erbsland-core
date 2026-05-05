// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteUnit.hpp"
#include "IntegerUnitOffset.hpp"

namespace erbsland::unit {

/// A byte-based offset.
using ByteOffset = IntegerUnitOffset<ByteUnit>;

}
