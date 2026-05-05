// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../math/SaturatingInteger_fwd.hpp"

#include <cstdint>
#include <ratio>

namespace erbsland::unit {

template <typename tUnit, typename tRatio, typename tValue = math::SaturatingInteger<std::int64_t>>
class IntegerAmount;

}
