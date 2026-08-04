// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../TimeAmounts.hpp"

#include <concepts>

namespace erbsland::time::impl {

/// A time amount supported by the DateTime tick conversion API.
template <typename T>
concept DateTimeTickUnit = std::same_as<T, Nanoseconds> || std::same_as<T, Microseconds> ||
    std::same_as<T, Milliseconds> || std::same_as<T, Seconds>;

}
