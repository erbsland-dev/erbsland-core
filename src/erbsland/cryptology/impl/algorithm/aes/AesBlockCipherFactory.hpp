// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AesBlockCipher.hpp"

#include "../../../../mem/ByteSpan.hpp"

#include <memory>

namespace erbsland::cryptology::impl {

/// Create the fastest permitted supported AES block cipher, with a portable fallback.
/// The application-wide acceleration setting is observed when this worker is constructed.
/// @param key The 16-byte or 32-byte AES key.
/// @return The selected AES implementation.
/// @tested{AesPrimitiveTest AesPrimitiveFullTest CryptologyConfigurationTest}
[[nodiscard]] auto createAesBlockCipher(mem::ConstByteSpan key) -> std::unique_ptr<AesBlockCipher>;
/// Create the portable AES implementation for reference testing.
/// @param key The 16-byte or 32-byte AES key.
/// @return The portable AES implementation.
/// @tested{AesPrimitiveTest}
[[nodiscard]] auto createPortableAesBlockCipher(mem::ConstByteSpan key) -> std::unique_ptr<AesBlockCipher>;

}
