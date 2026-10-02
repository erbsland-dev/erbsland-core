// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace app::constant_time {

/// Public operation selection for a fixed-shape experiment.
enum class StreamOperation {
    ChaCha20,         ///< Cha Cha20 operation.
    Poly1305,         ///< Poly1305 operation.
    AesGcm,           ///< Aes Gcm operation.
    ChaCha20Poly1305, ///< Cha Cha20Poly1305 operation.
};

}
