// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockPosition.hpp"
#include "BlockRectangle.hpp"
#include "BlockSize.hpp"

#include <format>

template <>
struct std::formatter<erbsland::bgeo::BlockPosition> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::bgeo::BlockPosition &pos, std::format_context &ctx) const
        -> std::format_context::iterator {
        return Base::format(std::format("{},{}", pos.x().toRawValue(), pos.y().toRawValue()), ctx);
    }
};

template <>
struct std::formatter<erbsland::bgeo::BlockSize> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::bgeo::BlockSize &size, std::format_context &ctx) const
        -> std::format_context::iterator {
        return Base::format(std::format("{}x{}", size.width().toRawValue(), size.height().toRawValue()), ctx);
    }
};

template <>
struct std::formatter<erbsland::bgeo::BlockRectangle> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::bgeo::BlockRectangle &rect, std::format_context &ctx) const
        -> std::format_context::iterator {
        return Base::format(std::format("{}:{}", rect.topLeft(), rect.size()), ctx);
    }
};
