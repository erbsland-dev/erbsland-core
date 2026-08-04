// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// A symmetric cipher family.
/// @seedoc{/reference/cryptology/symmetric_encryption}
enum class SymmetricCipher : uint8_t {
    None,     ///< No cipher family for an invalid encryption type.
    Aes,      ///< The Advanced Encryption Standard cipher family.
    ChaCha20, ///< The ChaCha20 stream cipher.
};

}
