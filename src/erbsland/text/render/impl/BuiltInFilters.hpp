// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BuiltInFilter.hpp"

#include "../Value_fwd.hpp"

#include "../../String_fwd.hpp"

#include <optional>

namespace erbsland::text::render::impl::built_in_filters {

/// Resolve the canonical or compatibility name of an ordinary built-in filter.
/// @tested{RenderLanguageCompletionTest RenderFilterTest}
[[nodiscard]] auto find(const String &name) noexcept -> std::optional<BuiltInFilter>;
/// Apply one ordinary built-in filter.
/// @param filter The built-in filter to execute.
/// @param value The input value.
/// @param arguments The additional filter arguments.
/// @return The filtered value.
/// @throws RenderError If the input or arguments are unsupported by the filter.
/// @tested{RenderLanguageCompletionTest}
[[nodiscard]] auto apply(BuiltInFilter filter, const Value &value, const ValueList &arguments) -> Value;

}
