// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyAgreementPrivateKey.hpp"

#include "../CryptologyError.hpp"
#include "../impl/key_agreement/KeyAgreementOperations.hpp"
#include "../impl/SecureEraseGuard.hpp"

#include "../../core/Application.hpp"
#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../mem/ByteArray.hpp"
#include "../../random/Random.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

KeyAgreementPrivateKey::KeyAgreementPrivateKey(
    const KeyAgreementAlgorithm algorithm, const mem::ConstByteSpan privateData, KeyAgreementPublicKey publicKey) :
    _algorithm{algorithm}, _privateData{privateData}, _publicKey{std::move(publicKey)} {
}

KeyAgreementPrivateKey::~KeyAgreementPrivateKey() {
    secureErase();
}

auto KeyAgreementPrivateKey::agree(const KeyAgreementPublicKey &peer) const -> KeyAgreementSharedSecret {
    if (isEmpty() || peer.isEmpty()) {
        throw err::LogicError{"Both private and peer public keys are required for key agreement."_el};
    }
    if (_algorithm != peer.algorithm()) {
        throw err::ParameterError{"Private and peer public-key algorithms must match."_el, "peer"_el};
    }
    auto result = KeyAgreementSharedSecret{};
    // Limit the private scalar's plaintext lifetime to this callback; ProtectedByteBlock erases it on every exit.
    _privateData.withUnprotectedData([&](const mem::ConstByteSpan privateData) -> void {
        auto sharedData = impl::key_agreement::agree(_algorithm, privateData, peer.span());

        // The raw shared coordinate is secret. Mark it before any validation that can throw and guard every exit.
        sharedData.markAsSensitive();
        const auto eraseGuard = impl::SecureEraseGuard{sharedData};

        // RFC 7748 section 6.1 permits the all-zero check without leaking which input caused it; RFC 8446 section
        // 7.4.2 requires rejection for X25519. ByteBlock performs the comparison in constant time.
        const auto zero = mem::ByteArray<32U>{};
        if (sharedData.isEqualConstTime(zero.span())) {
            throw CryptologyError{"Key agreement produced the forbidden all-zero shared secret."_el};
        }

        // Re-protect the accepted secret before sharedData and the unprotected private scalar leave scoped storage.
        result = KeyAgreementSharedSecret{_algorithm, sharedData.span()};
    });
    return result;
}

void KeyAgreementPrivateKey::secureErase() noexcept {
    _privateData.secureErase();
    _publicKey = {};
}

auto KeyAgreementPrivateKey::publicKey() const -> KeyAgreementPublicKey {
    if (isEmpty()) {
        throw err::LogicError{"A private key is required to access its public key."_el};
    }
    return _publicKey;
}

auto KeyAgreementPrivateKey::generate(const KeyAgreementAlgorithm algorithm) -> KeyAgreementPrivateKey {
    if (algorithm.privateKeySize().isZero()) {
        throw err::ParameterError{"A valid key-agreement algorithm is required."_el, "algorithm"_el};
    }
    // RFC 7748 section 6.1: generate 32 random X25519 private bytes. The algorithm implementation applies clamping
    // when the scalar is used; preserving the original bytes keeps import and generation behavior identical.
    auto privateData = core::application().secureRandom().buildByteBlock(algorithm.privateKeySize());
    privateData.markAsSensitive();
    // Erase the unprotected random bytes after fromBytes has calculated the public key and protected its copy.
    const auto eraseGuard = impl::SecureEraseGuard{privateData};
    return fromBytes(algorithm, privateData.span());
}

auto KeyAgreementPrivateKey::fromBytes(const KeyAgreementAlgorithm algorithm, const mem::ConstByteSpan data)
    -> KeyAgreementPrivateKey {
    if (algorithm.privateKeySize().isZero() || data.size() != algorithm.privateKeySize().toSizeT()) {
        throw err::ParameterError{"The private-key length does not match the key-agreement algorithm."_el, "data"_el};
    }
    // RFC 7748 section 6.1: K_A = X25519(a, 9). The caller retains ownership and erasure responsibility for data.
    const auto publicData = impl::key_agreement::publicKey(algorithm, data);
    return KeyAgreementPrivateKey{algorithm, data, KeyAgreementPublicKey{algorithm, publicData.span()}};
}

}
