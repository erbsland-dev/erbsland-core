// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::compression {

/// Policy for algorithms requiring complete byte values.
enum class CompressionFallbackPolicy {
    AllowBuffered,    ///< Allow one-shot codecs within the fallback buffer limits.
    RequireStreaming, ///< Reject unsupported streaming before consuming input.
};

}
