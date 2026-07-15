// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Helpers.hpp"

#include "../../../bgeo/BlockMargins.hpp"

#include <algorithm>

namespace erbsland::cterm::impl::document_renderer {

auto collapsedVerticalMarginValue(const bgeo::BlockCoordinate first, const bgeo::BlockCoordinate second) noexcept
    -> bgeo::BlockCoordinate {
    if (first >= 0 && second >= 0) {
        return std::max(first, second);
    }
    if (first <= 0 && second <= 0) {
        return std::min(first, second);
    }
    return first + second;
}

auto resolvedDecoration(
    BlockStringBuilder &builder, const std::optional<BlockStringView> &decoration, const BlockStyle textStyle)
    -> std::optional<BlockString> {
    if (!decoration.has_value()) {
        return std::nullopt;
    }
    builder.clear();
    builder.appendWithBaseStyle(*decoration, textStyle);
    return builder.toString();
}

auto listItemParagraphRule() noexcept -> TerminalDocumentStyleRule {
    auto rule = TerminalDocumentStyleRule{};
    rule.setIndents(ParagraphIndents{0, 0, 0, bgeo::BlockMargins{0}});
    return rule;
}

}
