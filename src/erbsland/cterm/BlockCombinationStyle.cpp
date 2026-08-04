// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockCombinationStyle.hpp"

#include "MatrixBlockCombinationStyle.hpp"
#include "SimpleBlockCombinationStyle.hpp"

#include "impl/CommonBoxFrameBlockCombinationStyle.hpp"

#include "../err/ParameterError.hpp"
#include "../unit/CpIndex.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace erbsland::cterm {

auto BlockCombinationStyle::combine([[maybe_unused]] const Block &current, const Block &overlay) const noexcept
    -> Block {
    return overlay;
}

auto BlockCombinationStyle::combine(
    [[maybe_unused]] const std::array<const Block *, 9> &current, const Block &overlay) const noexcept -> Block {
    return overlay;
}

auto BlockCombinationStyle::overwrite() noexcept -> const BlockCombinationStylePtr & {
    static const auto style = std::make_shared<BlockCombinationStyle>();
    return style;
}

auto BlockCombinationStyle::colorOverlay() noexcept -> const BlockCombinationStylePtr & {
    static const BlockCombinationStylePtr style = std::make_shared<SimpleBlockCombinationStyle>();
    return style;
}

auto BlockCombinationStyle::commonBoxFrame() noexcept -> const BlockCombinationStylePtr & {
    static const BlockCombinationStylePtr style = std::make_shared<impl::CommonBoxFrameBlockCombinationStyle>();
    return style;
}

}
