// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Assignment.hpp"

#include "../../../util/CoGenerator.hpp"

namespace erbsland::conf::impl {

/// The generator of the assignment stream.
using AssignmentGenerator = util::CoGenerator<Assignment>;

}
