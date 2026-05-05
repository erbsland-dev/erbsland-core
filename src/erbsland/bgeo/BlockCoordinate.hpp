// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../math/SaturatingInteger.hpp"

namespace erbsland::bgeo {

/// Represents a coordinate value with saturation arithmetic.
/// @tested{BlockCoordinateTest}
using BlockCoordinate = math::SatInt32;

}
