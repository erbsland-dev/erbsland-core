// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::compression {

/// Action requested by a progress observer.
enum class CompressionProgressAction {
    Continue, ///< Continue the operation.
    Cancel,   ///< Terminate an unfinished operation with a cancellation error.
};

}
