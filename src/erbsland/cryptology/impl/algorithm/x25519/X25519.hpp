// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../mem/ByteArray.hpp"
#include "../../../../mem/ByteSpan.hpp"

namespace erbsland::cryptology::impl::x25519 {

/// Derive an X25519 public key from 32 private bytes.
[[nodiscard]] auto publicKey(mem::ConstByteSpan privateKey) -> mem::ByteArray<32U>;
/// Calculate an X25519 shared coordinate from exact private and peer bytes.
[[nodiscard]] auto agree(mem::ConstByteSpan privateKey, mem::ConstByteSpan peerPublicKey) -> mem::ByteArray<32U>;
/// Execute the X25519 Montgomery ladder from RFC 7748 section 5.
[[nodiscard]] auto scalarMultiply(mem::ConstByteSpan scalarBytes, mem::ConstByteSpan coordinateBytes)
    -> mem::ByteArray<32U>;

}
