// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace app::constant_time {

/// Public operation selection for a fixed-shape experiment.
enum class RsaOperation {
    Reduce,     ///< Reduce operation.
    Multiply,   ///< Multiply operation.
    Subtract,   ///< Subtract operation.
    Power,      ///< Power operation.
    SignSha256, ///< Sign Sha256 operation.
    SignSha384, ///< Sign Sha384 operation.
};

}
