// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace app::constant_time {

/// Public operation selection for a fixed-shape experiment.
enum class ValidationOperation {
    Aes128Gcm,        ///< Aes128Gcm operation.
    Aes256Gcm,        ///< Aes256Gcm operation.
    ChaCha20Poly1305, ///< Cha Cha20Poly1305 operation.
    CbcPadding,       ///< Cbc Padding operation.
};

}
