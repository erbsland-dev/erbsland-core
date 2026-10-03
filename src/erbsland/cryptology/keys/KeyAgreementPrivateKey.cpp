// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyAgreementPrivateKey.hpp"

#include "../CryptologyError.hpp"
#include "../impl/key_agreement/KeyAgreementOperations.hpp"
#include "../impl/key_agreement/P256KeyAgreement.hpp"
#include "../impl/SecureEraseGuard.hpp"

#include "../../core/impl/application_data/ApplicationRandomData.hpp"
#include "../../core/impl/ApplicationDataAccess.hpp"
#include "../../err/LogicFailure.hpp"
#include "../../err/OutOfRangeError.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../mem/ByteArray.hpp"
#include "../../random/Random.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace err;
using namespace mem;
using namespace text::literals;

KeyAgreementPrivateKey::KeyAgreementPrivateKey(
    const KeyAgreementAlgorithm algorithm, const ConstByteSpan privateData, KeyAgreementPublicKey publicKey) :
    _algorithm{algorithm}, _privateData{privateData}, _publicKey{std::move(publicKey)} {
}

KeyAgreementPrivateKey::~KeyAgreementPrivateKey() {
    secureErase();
}

auto KeyAgreementPrivateKey::agree(const KeyAgreementPublicKey &peer) const -> KeyAgreementSharedSecret {
    if (isEmpty() || peer.isEmpty()) {
        throw LogicFailure{"Both private and peer public keys are required for key agreement."_el};
    }
    if (_algorithm != peer.algorithm()) {
        throw ParameterFailure{"Private and peer public-key algorithms must match."_el, "peer"_el};
    }
    auto result = KeyAgreementSharedSecret{};
    // Limit the private scalar's plaintext lifetime to this callback; ProtectedByteBlock erases it on every exit.
    _privateData.withUnprotectedData([&](const ConstByteSpan privateData) -> void {
        auto sharedData = impl::key_agreement::agree(_algorithm, privateData, peer.span());

        // The raw shared coordinate is secret. Mark it before any validation that can throw and guard every exit.
        sharedData.markAsSensitive();
        const auto eraseGuard = impl::SecureEraseGuard{sharedData};

        // RFC 7748 section 6.1 permits the all-zero check without leaking which input caused it; RFC 8446 section
        // 7.4.2 requires rejection for X25519. ByteBlock performs the comparison in constant time.
        const auto zero = ByteArray<32U>{};
        if (_algorithm == KeyAgreementAlgorithm::X25519 && sharedData.isEqualConstTime(zero.span())) {
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
        throw LogicFailure{"A private key is required to access its public key."_el};
    }
    return _publicKey;
}

auto KeyAgreementPrivateKey::generate(const KeyAgreementAlgorithm algorithm) -> KeyAgreementPrivateKey {
    if (algorithm.privateKeySize().isZero()) {
        throw ParameterFailure{"A valid key-agreement algorithm is required."_el, "algorithm"_el};
    }
    // SEC 1 section 3.2.1: rejection-sample P-256 in [1,n-1], never reduce a random scalar modulo n.
    // RFC 7748 section 6.1: all 32-byte X25519 strings are accepted; clamping happens at use.
    // The 128-attempt limit detects a malfunctioning randomness provider, not a biased fallback.
    const auto randomData = core::impl::applicationData()->random().get();
    for (auto attempt = 0U; attempt < 128U; ++attempt) {
        auto privateData = randomData->secureRandom().buildByteBlock(algorithm.privateKeySize());
        privateData.markAsSensitive();
        const auto eraseGuard = impl::SecureEraseGuard{privateData};
        if (algorithm == KeyAgreementAlgorithm::P256 &&
            !impl::p256_key_agreement::isValidPrivateKey(privateData.span())) {
            continue;
        }
        return fromBytes(algorithm, privateData.span());
    }
    throw CryptologyError{"Failed to sample a valid key-agreement scalar."_el};
}

auto KeyAgreementPrivateKey::fromBytes(const KeyAgreementAlgorithm algorithm, const ConstByteSpan data)
    -> KeyAgreementPrivateKey {
    if (algorithm.privateKeySize().isZero()) {
        throw ParameterFailure{"A valid key-agreement algorithm is required."_el, "algorithm"_el};
    }
    if (data.size() != algorithm.privateKeySize().toSizeT()) {
        throw OutOfRangeError{"The private-key length does not match the key-agreement algorithm."_el};
    }
    // RFC 7748 section 6.1 / SEC 1 section 3.2.1: derive the public key. The caller retains erasure ownership of data.
    const auto publicData = impl::key_agreement::publicKey(algorithm, data);
    return KeyAgreementPrivateKey{algorithm, data, KeyAgreementPublicKey{algorithm, publicData.span()}};
}

}
