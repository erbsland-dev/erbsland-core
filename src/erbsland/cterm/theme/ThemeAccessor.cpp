// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ThemeAccessor.hpp"

#include <utility>

namespace erbsland::cterm::theme {

auto ThemeAccessor::forPart(const Part part) const noexcept -> ThemeAccessor {
    return ThemeAccessor{_theme, _selector.withPart(part)};
}

auto ThemeAccessor::withStates(const States states) const noexcept -> ThemeAccessor {
    return ThemeAccessor{_theme, _selector.withStates(_selector.states() | states)};
}

auto ThemeAccessor::withTags(const Tags tags) const noexcept -> ThemeAccessor {
    return ThemeAccessor{_theme, _selector.withTags(_selector.tags() | tags)};
}

}
