// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../RenderError.hpp"

#include "../../Literals.hpp"

#include <utility>

namespace erbsland::text::render::impl::built_in_filters {

/// Report invalid built-in filter input; the renderer adds the calling layout's location.
/// @param message The rejected input or argument description.
/// @throws RenderError Always, with category Runtime.
/// @tested{RenderFilterTest RenderLanguageCompletionTest}
[[noreturn]] inline void throwInputError(String message) {
    using namespace text::literals;
    throw RenderError{
        RenderErrorContext{RenderErrorCategory::Runtime, "Invalid built-in filter input"_el, std::move(message)}};
}

}
