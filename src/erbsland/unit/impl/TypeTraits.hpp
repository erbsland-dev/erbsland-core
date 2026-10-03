// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IntegerUnit_fwd.hpp"

#include <concepts>

namespace erbsland::unit::impl {

/// A tag identifying a positional integer unit.
template <typename T>
concept ValidIntegerUnit = std::derived_from<T, IntegerUnit>;

}
