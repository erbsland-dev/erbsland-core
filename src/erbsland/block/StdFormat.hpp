// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Position.hpp"
#include "Rectangle.hpp"
#include "Size.hpp"

#include <format>

template <>
struct std::formatter<erbsland::block::Position> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::block::Position &pos, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(std::format("{},{}", pos.x().toRawValue(), pos.y().toRawValue()), ctx);
    }
};

template <>
struct std::formatter<erbsland::block::Size> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::block::Size &size, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(std::format("{}x{}", size.width().toRawValue(), size.height().toRawValue()), ctx);
    }
};

template <>
struct std::formatter<erbsland::block::Rectangle> : std::formatter<std::string> {
    using Base = std::formatter<std::string>;

    auto format(const erbsland::block::Rectangle &rect, std::format_context &ctx) const
        -> std::format_context::iterator {
        return Base::format(std::format("{}:{}", rect.topLeft(), rect.size()), ctx);
    }
};
