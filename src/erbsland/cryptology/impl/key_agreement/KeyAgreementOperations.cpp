// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyAgreementOperations.hpp"

#include "../algorithm/x25519/X25519.hpp"
#include "../SecureEraseGuard.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

namespace erbsland::cryptology::impl::key_agreement {

using namespace text::literals;

auto publicKey(const KeyAgreementAlgorithm algorithm, const mem::ConstByteSpan privateKey) -> mem::ByteBlock {
    switch (algorithm.toRawValue()) {
    case KeyAgreementAlgorithm::X25519:
        return mem::ByteBlock{x25519::publicKey(privateKey)};
    }
    throw CryptologyError{"No implementation exists for the key-agreement algorithm."_el};
}

auto agree(
    const KeyAgreementAlgorithm algorithm, const mem::ConstByteSpan privateKey, const mem::ConstByteSpan peerPublicKey)
    -> mem::ByteBlock {
    switch (algorithm.toRawValue()) {
    case KeyAgreementAlgorithm::X25519:
        // RFC 7748 section 6.1: K = X25519(privateKey, peerPublicKey). Erase the fixed stack representation after
        // transferring its bytes into sensitive owning storage.
        auto coordinate = x25519::agree(privateKey, peerPublicKey);
        const auto coordinateEraseGuard = SecureEraseGuard{coordinate};
        auto result = mem::ByteBlock{coordinate};
        result.markAsSensitive();
        return result;
    }
    throw CryptologyError{"No implementation exists for the key-agreement algorithm."_el};
}

}
