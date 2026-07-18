// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BlockStringBuilder.hpp"

#include "../../../bgeo/BlockCoordinate.hpp"
#include "../../BlockString.hpp"
#include "../../BlockStringEditor.hpp"
#include "../../BlockStyle.hpp"
#include "../../ParagraphIndents.hpp"
#include "../../TerminalDocumentStyleRule.hpp"

#include <optional>

namespace erbsland::cterm::impl::document_renderer {

/// Collapse two vertical margin values.
/// @param first The first margin.
/// @param second The second margin.
/// @return The collapsed margin.
[[nodiscard]] auto collapsedVerticalMarginValue(bgeo::BlockCoordinate first, bgeo::BlockCoordinate second) noexcept
    -> bgeo::BlockCoordinate;

/// Resolve an optional decoration with the given base style.
/// @param builder The scratch builder to reuse.
/// @param decoration The decoration text.
/// @param textStyle The base text style.
/// @return The styled decoration, or no value.
[[nodiscard]] auto resolvedDecoration(
    BlockStringBuilder &builder, const std::optional<BlockString> &decoration, BlockStyle textStyle)
    -> std::optional<BlockString>;

/// Create the neutral paragraph rule used for list-item text blocks.
/// @return The neutral paragraph rule.
[[nodiscard]] auto listItemParagraphRule() noexcept -> TerminalDocumentStyleRule;

}
