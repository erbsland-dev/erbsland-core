// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Value_fwd.hpp"

#include <compare>
#include <cstdint>

namespace erbsland::text::render::impl::value_comparison {

/// Compare one signed integer with one floating-point value without lossy integer conversion.
/// @tested{RenderExpressionTest RenderLanguageCompletionTest}
[[nodiscard]] auto compareIntegerFloat(int64_t left, double right) -> std::partial_ordering;

/// Reverse a partial ordering while preserving its unordered state.
/// @tested{RenderExpressionTest RenderLanguageCompletionTest}
[[nodiscard]] auto reverseOrdering(std::partial_ordering value) noexcept -> std::partial_ordering;

/// Compare any supported integer/float pairing without lossy integer conversion.
/// @tested{RenderExpressionTest RenderLanguageCompletionTest}
[[nodiscard]] auto compareNumbers(const Value &left, const Value &right) -> std::partial_ordering;

}
