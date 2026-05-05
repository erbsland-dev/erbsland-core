// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>

namespace erbsland::text::impl {

/// Concept for floating point types supported by the public parse API.
template <typename T>
concept AnyFloatType = std::same_as<T, float> || std::same_as<T, double>;

}
