// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../BlockIndex.hpp"
#include "../../BlockRange.hpp"

#include <optional>
#include <vector>

namespace erbsland::cterm::impl::paragraph {

/// Optional semantic boundaries for paragraph source text.
///
/// Indices always refer to the unmodified paragraph source. Soft breaks split adjacent words without producing a
/// visible character. Indivisible ranges are emitted as a unit and are never split by the paragraph layout.
/// @tested{TerminalDocumentRendererTest}
struct LayoutSemantics final {
    std::vector<BlockIndex> softBreaks;        ///< Boundaries where a line may wrap without visible spacing.
    std::vector<BlockRange> indivisibleRanges; ///< Source ranges that must remain intact.

    /// Test whether a soft break exists before the given source character.
    /// @param index The source character index.
    /// @return `true` if a semantic soft break exists at `index`.
    [[nodiscard]] auto hasSoftBreak(BlockIndex index) const noexcept -> bool;
    /// Find an indivisible range starting at the given source character.
    /// @param index The source character index.
    /// @return The matching range, or `std::nullopt`.
    [[nodiscard]] auto indivisibleRangeAt(BlockIndex index) const noexcept -> std::optional<BlockRange>;
};

}
