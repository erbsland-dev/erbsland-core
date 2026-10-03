// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ArgumentUnit.hpp"
#include "IntegerUnitCount.hpp"

namespace erbsland::unit {

/// A non-negative number of arguments.
using ArgumentCount = IntegerUnitCount<ArgumentUnit>;

}
