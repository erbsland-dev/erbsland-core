// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteSpan.hpp"

#include <array>
#include <cstdint>

namespace erbsland::cryptology::impl::chacha20 {

/// The sixteen 32-bit words of one RFC 8439 ChaCha20 state.
/// @tested{ChaCha20PrimitiveTest}
using State = std::array<uint32_t, 16>;

/// Apply the RFC 8439 Section 2.1.1 ChaCha quarter round to four state words.
/// @param state The state to update.
/// @param a The first word index.
/// @param b The second word index.
/// @param c The third word index.
/// @param d The fourth word index.
/// @tested{ChaCha20PrimitiveTest}
void quarterRound(State &state, std::size_t a, std::size_t b, std::size_t c, std::size_t d) noexcept;

/// Build the RFC 8439 Section 2.3 ChaCha20 initial state.
/// @param key The 256-bit key.
/// @param nonce The 96-bit nonce.
/// @param counter The 32-bit block counter.
/// @return The initialized state.
/// @tested{ChaCha20PrimitiveTest}
[[nodiscard]] auto initialState(mem::ConstByteSpan key, mem::ConstByteSpan nonce, uint32_t counter) noexcept -> State;

/// Generate one RFC 8439 Section 2.3 ChaCha20 block.
/// @param key The 256-bit key.
/// @param nonce The 96-bit nonce.
/// @param counter The 32-bit block counter.
/// @return The 64-byte key-stream block.
/// @tested{ChaCha20PrimitiveTest}
[[nodiscard]] auto block(mem::ConstByteSpan key, mem::ConstByteSpan nonce, uint32_t counter) noexcept
    -> mem::ByteArray<64>;

}
