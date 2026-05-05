// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerUnitAmount.hpp"
#include "U16DataUnit.hpp"

namespace erbsland::unit {

/// A length for UTF-16 data.
using U16DataLength = IntegerUnitAmount<U16DataUnit>;

}
