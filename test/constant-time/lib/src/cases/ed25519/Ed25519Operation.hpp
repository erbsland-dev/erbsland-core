// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace app::constant_time {

/// Public operation selection for a fixed-shape experiment.
enum class Ed25519Operation {
    PublicKey,     ///< Public Key operation.
    Sign,          ///< Sign operation.
    Reduce,        ///< Reduce operation.
    Add,           ///< Add operation.
    Multiply,      ///< Multiply operation.
    BaseMultiply,  ///< Base Multiply operation.
    FieldMultiply, ///< Field Multiply operation.
    FieldInvert,   ///< Field Invert operation.
};

}
