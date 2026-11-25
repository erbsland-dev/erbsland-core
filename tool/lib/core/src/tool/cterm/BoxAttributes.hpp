// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <cstdint>

namespace tool::cterm {

/// Seven stroke attributes in runtime packing order.
/// @tested{BoxFrameToolTest}
struct BoxAttributes {
    std::array<uint8_t, 7> strokes{}; ///< Right, down, left, up, slash, backslash, center.

    friend auto operator==(const BoxAttributes &, const BoxAttributes &) noexcept -> bool = default;
    friend auto operator<(const BoxAttributes &left, const BoxAttributes &right) noexcept -> bool {
        return left.strokes < right.strokes;
    }
};

}
