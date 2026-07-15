// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitOffset.hpp"
#include "LineUnit.hpp"

namespace erbsland::unit {

/// A signed source code line offset.
using LineOffset = IntegerUnitOffset<LineUnit>;

}
