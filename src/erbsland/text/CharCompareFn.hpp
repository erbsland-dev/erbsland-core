// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char_fwd.hpp"

#include <compare>

namespace erbsland::text {

/// A function to compare two decoded characters.
using CharCompareFn = std::strong_ordering (*)(Char left, Char right) noexcept;

}
