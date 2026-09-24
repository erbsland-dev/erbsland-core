// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression {

/// A portable, human-facing compression effort level.
enum class CompressionLevel : uint8_t {
    Fastest, ///< Minimize compression time.
    Fast,    ///< Prefer compression speed over size.
    Default, ///< Use the balanced codec-specific default.
    High,    ///< Prefer compressed size over speed.
    Highest, ///< Use the highest supported compression effort.
};

}
