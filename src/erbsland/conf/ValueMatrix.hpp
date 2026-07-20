// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Matrix.hpp"
#include "Value_fwd.hpp"

namespace erbsland::conf {

using ValueMatrix = Matrix<ValuePtr>;
using ConstValueMatrix = Matrix<ConstValuePtr>;

}
