// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace tool::unicode {

/// One simple case-mapping delta triple.
/// @tested{UnicodeDataToolTest}
struct LightDelta {
    int32_t fold{};  ///< Simple fold delta.
    int32_t lower{}; ///< Simple lowercase delta.
    int32_t upper{}; ///< Simple uppercase delta.

    friend auto operator==(const LightDelta &, const LightDelta &) noexcept -> bool = default;
};

}
