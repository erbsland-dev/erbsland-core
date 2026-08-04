// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ChaCha20Backend.hpp"

#include <memory>

namespace erbsland::cryptology::impl {

/// Create the best permitted ChaCha20 backend for the current application configuration.
/// @param key The validated 256-bit key.
/// @param nonce The validated 96-bit nonce.
/// @return The selected backend.
/// @tested{ChaCha20PrimitiveTest ChaCha20BackendFullTest}
[[nodiscard]] auto createChaCha20Backend(mem::ConstByteSpan key, mem::ConstByteSpan nonce)
    -> std::unique_ptr<ChaCha20Backend>;

/// Create the portable ChaCha20 backend explicitly.
/// @param key The validated 256-bit key.
/// @param nonce The validated 96-bit nonce.
/// @return The portable backend.
/// @tested{ChaCha20PrimitiveTest ChaCha20BackendFullTest}
[[nodiscard]] auto createPortableChaCha20Backend(mem::ConstByteSpan key, mem::ConstByteSpan nonce)
    -> std::unique_ptr<ChaCha20Backend>;

}
