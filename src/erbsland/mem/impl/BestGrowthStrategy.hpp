// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::mem::impl {

/// Select how a growing allocation reserves spare capacity.
enum class BestGrowthStrategy {
    Compact,  ///< Minimize spare capacity using compact allocation blocks.
    Geometric ///< Double larger allocations to avoid repeated relocation.
};

}
