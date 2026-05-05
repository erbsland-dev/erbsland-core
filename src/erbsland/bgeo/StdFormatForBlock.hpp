// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockPosition_fwd.hpp"
#include "BlockRectangle_fwd.hpp"
#include "BlockSize_fwd.hpp"

#include <format>

template <>
struct std::formatter<erbsland::bgeo::BlockPosition> : std::formatter<std::string> {
    auto format(const erbsland::bgeo::BlockPosition &pos, std::format_context &ctx) const
        -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::bgeo::BlockRectangle> : std::formatter<std::string> {
    auto format(const erbsland::bgeo::BlockRectangle &rect, std::format_context &ctx) const
        -> std::format_context::iterator;
};

template <>
struct std::formatter<erbsland::bgeo::BlockSize> : std::formatter<std::string> {
    auto format(const erbsland::bgeo::BlockSize &size, std::format_context &ctx) const -> std::format_context::iterator;
};
