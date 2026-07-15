// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::path {

/// The returned status of a file walk function.
enum class PathWalkStatus : uint8_t {
    Continue, ///< Continue with the next path.
    Skip,     ///< Skip descent in root-to-leaf order; equivalent to `Continue` in leaf-to-root order.
    Stop,     ///< Stop the walk.
    Failure,  ///< Stop the walk because of a failure.
};

}
