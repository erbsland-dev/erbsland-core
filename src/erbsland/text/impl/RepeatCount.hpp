// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ThrowHelper.hpp"

#include <cstddef>

namespace erbsland::text::impl {

/// Convert a bounded repeat count to a native storage size.
template <typename T>
[[nodiscard]] auto repeatCountToSize(const T count) -> std::size_t {
    if (count.isInfinite()) {
        throwOverflow("Repeated string count must be finite");
    }
    return count.toSizeTOrThrow();
}

}
