// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>
#include <vector>

namespace erbsland::conf::impl {

class Constraint;
using ConstraintPtr = std::shared_ptr<Constraint>;
using ConstraintList = std::vector<ConstraintPtr>;

}
