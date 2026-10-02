// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace app::constant_time {

/// Public operation selection for a fixed-shape experiment.
enum class EcdsaOperation {
    PublicKey,    ///< Public Key operation.
    Sign,         ///< Sign operation.
    Multiply,     ///< Multiply operation.
    Invert,       ///< Invert operation.
    Add,          ///< Add operation.
    BaseMultiply, ///< Base Multiply operation.
};

}
