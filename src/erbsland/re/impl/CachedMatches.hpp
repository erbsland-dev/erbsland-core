// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../RegEx.hpp"

#include "../../util/CoGenerator.hpp"

namespace erbsland::re::impl {

/// Lazily yield matches while retaining the expression and subject in the coroutine frame.
/// @tparam T The owning Core string type.
/// @param expression Owning compiled expression.
/// @param text Owning subject string.
/// @return A single-pass match generator.
/// @throws RegExError If matching fails.
/// @tested{RegExCachedTest}
template <typename T>
[[nodiscard]] auto cachedMatches(ConstRegExPtr expression, T text) -> decltype(expression->findAll(text)) {
    for (const auto &match : expression->findAll(text)) {
        co_yield match;
    }
}

}
