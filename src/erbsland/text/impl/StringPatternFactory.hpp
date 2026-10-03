// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StaticStringPatternData.hpp"
#include "StringPatternData.hpp"

#include "../AnyString_fwd.hpp"
#include "../pattern/AnyElement.hpp"
#include "../pattern/Limits.hpp"
#include "../StringPatternKind.hpp"

#include <memory>

namespace erbsland::text::impl {

/// Create runtime pattern data from text of any supported width.
/// @param pattern The pattern text.
/// @param kind The syntax and matching semantics.
/// @return The compiled pattern data.
/// @throws ParseError If the pattern is invalid or exceeds storage limits.
/// @tested{StringPatternTest}
[[nodiscard]] auto createStringPatternData(const AnyString &pattern, StringPatternKind kind) -> StringPatternDataPtr;

/// Create static pattern data from pattern elements.
/// @tparam Args The supported element types.
/// @param kind Divider or Glob matching semantics.
/// @param elements The pattern elements.
/// @return The compiled pattern data.
/// @throws ParseError If the pattern is invalid or exceeds storage limits.
/// @tested{StringPatternTest}
template <pattern::AnyElement... Args>
[[nodiscard]] auto createStaticStringPatternData(const StringPatternKind kind, const Args &...elements)
    -> StringPatternDataPtr {
    using Data = StaticStringPatternData<pattern::cMaximumStaticElements, pattern::cMaximumStaticRanges>;
    return std::make_shared<Data>(kind, elements...);
}

}
