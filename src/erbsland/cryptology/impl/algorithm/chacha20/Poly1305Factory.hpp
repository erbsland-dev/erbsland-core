// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Poly1305.hpp"

#include <memory>

namespace erbsland::cryptology::impl {

/// Create the best permitted Poly1305 backend for the current application configuration.
/// @param key The validated 256-bit one-time key.
/// @return The selected backend.
/// @tested{ChaCha20PrimitiveTest ChaCha20BackendFullTest}
[[nodiscard]] auto createPoly1305(mem::ConstByteSpan key) -> std::unique_ptr<Poly1305>;

/// Create the portable Poly1305 backend explicitly.
/// @param key The validated 256-bit one-time key.
/// @return The portable backend.
/// @tested{ChaCha20PrimitiveTest ChaCha20BackendFullTest}
[[nodiscard]] auto createPortablePoly1305(mem::ConstByteSpan key) -> std::unique_ptr<Poly1305>;

}
