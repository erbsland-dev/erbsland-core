// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StdFormatForBlock.hpp"

#include "BlockPosition.hpp"
#include "BlockRectangle.hpp"
#include "BlockSize.hpp"

auto std::formatter<erbsland::bgeo::BlockPosition>::format(
    const erbsland::bgeo::BlockPosition &pos, std::format_context &ctx) const -> std::format_context::iterator {
    return std::formatter<std::string>::format(std::format("{},{}", pos.x().toRawValue(), pos.y().toRawValue()), ctx);
}

auto std::formatter<erbsland::bgeo::BlockRectangle>::format(
    const erbsland::bgeo::BlockRectangle &rect, std::format_context &ctx) const -> std::format_context::iterator {
    return std::formatter<std::string>::format(std::format("{}:{}", rect.topLeft(), rect.size()), ctx);
}

auto std::formatter<erbsland::bgeo::BlockSize>::format(
    const erbsland::bgeo::BlockSize &size, std::format_context &ctx) const -> std::format_context::iterator {
    return std::formatter<std::string>::format(
        std::format("{}x{}", size.width().toRawValue(), size.height().toRawValue()), ctx);
}
