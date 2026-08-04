// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeSnippetLayoutCell.hpp"

#include "../Char.hpp"

namespace erbsland::text::impl {

auto CodeSnippetLayoutCell::ellipsis() -> CodeSnippetLayoutCell {
    return {String::fromCharacter(Char{U'…'}), {}, 1, {}, true};
}

auto CodeSnippetLayoutCell::overlaps(const unit::ColumnRange other) const noexcept -> bool {
    return !isEllipsis && !other.isEmpty() && range.index() < other.endIndex() && other.index() < range.endIndex();
}

auto CodeSnippetLayoutCell::containsOrFollows(const unit::ColumnIndex point) const noexcept -> bool {
    return !isEllipsis && (point <= range.index() || range.contains(point));
}

}
