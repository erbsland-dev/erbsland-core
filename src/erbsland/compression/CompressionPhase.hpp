// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::compression {

/// Current transfer phase.
enum class CompressionPhase {
    Processing, ///< Input and output are being transferred.
    Finalizing, ///< The representation is being completed and validated.
    Completed,  ///< All output was accepted and validation succeeded.
};

}
