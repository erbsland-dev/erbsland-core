// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyAgreementOperations.hpp"

#include "P256KeyAgreement.hpp"

#include "../algorithm/x25519/X25519.hpp"
#include "../SecureEraseGuard.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../../text/Literals.hpp"
#include "../../CryptologyError.hpp"

namespace erbsland::cryptology::impl::key_agreement {

using namespace mem;
using namespace text::literals;

auto publicKey(const KeyAgreementAlgorithm algorithm, const ConstByteSpan privateKey) -> ByteBlock {
    switch (algorithm.toRawValue()) {
    case KeyAgreementAlgorithm::P256:
        return p256_key_agreement::publicKey(privateKey);
    case KeyAgreementAlgorithm::X25519:
        return ByteBlock{x25519::publicKey(privateKey)};
    }
    throw CryptologyError{"No implementation exists for the key-agreement algorithm."_el};
}

auto agree(const KeyAgreementAlgorithm algorithm, const ConstByteSpan privateKey, const ConstByteSpan peerPublicKey)
    -> ByteBlock {
    switch (algorithm.toRawValue()) {
    case KeyAgreementAlgorithm::P256:
        return p256_key_agreement::agree(privateKey, peerPublicKey);
    case KeyAgreementAlgorithm::X25519:
        // RFC 7748 section 6.1: K = X25519(privateKey, peerPublicKey). Erase the fixed stack representation after
        // transferring its bytes into sensitive owning storage.
        auto coordinate = x25519::agree(privateKey, peerPublicKey);
        const auto coordinateEraseGuard = SecureEraseGuard{coordinate};
        auto result = ByteBlock{coordinate};
        result.markAsSensitive();
        return result;
    }
    throw CryptologyError{"No implementation exists for the key-agreement algorithm."_el};
}

}
