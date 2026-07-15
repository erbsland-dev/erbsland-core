// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitIndex.hpp"
#include "LineUnit.hpp"

namespace erbsland::unit {

/// A zero-based source code line index.
using LineIndex = IntegerUnitIndex<LineUnit>;

}
