// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstddef>

namespace erbsland::text::pattern {

/// Maximum number of elements in a typed lightweight string pattern.
constexpr auto cMaximumStaticElements = std::size_t{16U};
/// Maximum number of total ranges in a typed lightweight string pattern.
constexpr auto cMaximumStaticRanges = std::size_t{16U};
/// Maximum number of elements in a typed lightweight string pattern.
constexpr auto cMaximumElements = cMaximumStaticElements;
/// Maximum number of ranges in one set element.
constexpr auto cMaximumSetRanges = std::size_t{8U};

}
