// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../keys/KeyAgreementAlgorithm.hpp"

namespace erbsland::cryptology::impl::key_agreement {

/// Dispatch generic key-agreement operations to algorithm implementations.
/// Derive an encoded public key.
[[nodiscard]] auto publicKey(KeyAgreementAlgorithm algorithm, mem::ConstByteSpan privateKey) -> mem::ByteBlock;
/// Calculate an encoded shared secret.
[[nodiscard]] auto agree(
    KeyAgreementAlgorithm algorithm, mem::ConstByteSpan privateKey, mem::ConstByteSpan peerPublicKey) -> mem::ByteBlock;

}
