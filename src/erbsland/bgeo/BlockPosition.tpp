// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>

namespace erbsland::bgeo {

template <typename Fn>
    requires std::invocable<Fn, BlockPosition> && std::convertible_to<std::invoke_result_t<Fn, BlockPosition>, bool>
auto BlockPosition::cardinalFourBitmask(Fn fn) const noexcept -> uint32_t {
    const auto &deltas = cardinalFourDeltas();
    return std::accumulate(
        deltas.rbegin(), deltas.rend(), 0U, [&](const uint32_t acc, const BlockPosition delta) -> uint32_t {
            return (acc << 1) | (fn(*this + delta) ? 1 : 0);
        });
}

}
