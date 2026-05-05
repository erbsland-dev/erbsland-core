// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockStringWithMargins.hpp"

namespace erbsland::cterm::theme {

void BlockStringWithMargins::join(const BlockStringWithMargins &other) noexcept {
    const auto collapsedInnerMargins = std::max(_margins.right(), other._margins.left());
    _text.append(BlockCount::fromSizeT(collapsedInnerMargins.toSizeT()), Block::space());
    _text.append(other._text);
    _margins = bgeo::BlockMargins{
        std::max(_margins.top(), other._margins.top()),
        other._margins.right(),
        std::max(_margins.bottom(), other._margins.bottom()),
        _margins.left()};
}

auto BlockStringWithMargins::joined(const BlockStringWithMargins &other) const noexcept -> BlockStringWithMargins {
    auto text = _text;
    const auto collapsedInnerMargins = std::max(_margins.right(), other._margins.left());
    text.append(BlockCount::fromSizeT(collapsedInnerMargins.toSizeT()), Block::space());
    text.append(other._text);
    return BlockStringWithMargins{
        text,
        bgeo::BlockMargins{
            std::max(_margins.top(), other._margins.top()),
            other._margins.right(),
            std::max(_margins.bottom(), other._margins.bottom()),
            _margins.left()}};
}

}
