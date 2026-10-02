// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DateTimeTraits.hpp"

#include "../Duration_fwd.hpp"
#include "../TimeDelta_fwd.hpp"

namespace erbsland::time::impl {

/// A fixed amount accepted by timestamp arithmetic.
/// @tested{TimestampArithmeticTest}
template <typename T>
concept TimestampAmount = DateTimeTickUnit<T> || std::same_as<T, Duration> || std::same_as<T, TimeDelta>;

}
