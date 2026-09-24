// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZipOperationPhase.hpp"

#include "../CompressionProgress.hpp"

#include "../../path/Path.hpp"

namespace erbsland::compression::zip {
/// Progress of one ZIP entry operation.
/// @tested{CompressionStreamingTest ZipArchiveTest}
struct ZipProgress final {
    CompressionProgress transfer; ///< Cumulative entry byte counts.
    path::Path itemPath;          ///< Normalized path inside the archive.
    ZipOperationPhase phase;      ///< Archive operation producing this update.
};
/// Synchronous entry observer with cancellation support.
using ZipProgressFn = std::function<CompressionProgressAction(const ZipProgress &)>;
}
