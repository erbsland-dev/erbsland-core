// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SigningKeyGenerator.hpp"

#include "CryptologyOids.hpp"
#include "DerEncoder.hpp"
#include "SecureEraseGuard.hpp"
#include "SigningKeyEncoding.hpp"

#include "algorithm/ecdsa_signature/EcdsaSigner.hpp"
#include "algorithm/rsa_signature/RsaKeyGenerator.hpp"
#include "algorithm/rsa_signature/RsaSigner.hpp"

#include "../CryptologyError.hpp"
#include "../keys/PublicKey.hpp"
#include "../keys/SigningPrivateKey.hpp"

#include "../../core/Application.hpp"
#include "../../random/Random.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

auto SigningKeyGenerator::generate() const -> SigningPrivateKey {
    switch (_profile) {
    case SigningKeyProfile::EcdsaP256:
    case SigningKeyProfile::EcdsaP384:
        return generateEc();
    case SigningKeyProfile::Rsa2048:
    case SigningKeyProfile::Rsa3072:
    case SigningKeyProfile::Rsa4096:
        return generateRsa();
    }
    throw CryptologyError{"Unknown signing-key generation profile."_el};
}

auto SigningKeyGenerator::generateRsa() const -> SigningPrivateKey {
    const auto bits = _profile == SigningKeyProfile::Rsa2048 ? std::size_t{2048U}
        : _profile == SigningKeyProfile::Rsa3072             ? std::size_t{3072U}
                                                             : std::size_t{4096U};
    auto privateKey = rsa_key_generator::generate(bits);
    const auto privateKeyEraseGuard = SecureEraseGuard{privateKey};
    auto encoder = DerEncoder{};
    const auto algorithmScope = encoder.beginSequence();
    encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::rsaEncryption));
    encoder.appendNull();
    encoder.end(algorithmScope);
    const auto algorithm = encoder.encoded();
    auto publicKeyEncoder = DerEncoder{};
    rsa_signer::appendPublicKey(publicKeyEncoder, privateKey.span(), algorithm.span());
    const auto publicKey = PublicKey::fromDerOrThrow(publicKeyEncoder.encoded());
    return SigningPrivateKey{SigningKeyAlgorithm::Rsa, privateKey.span(), publicKey};
}

auto SigningKeyGenerator::generateEc() const -> SigningPrivateKey {
    const auto curveName =
        _profile == SigningKeyProfile::EcdsaP256 ? NistPrimeCurve::Name::P256 : NistPrimeCurve::Name::P384;
    const auto algorithm =
        _profile == SigningKeyProfile::EcdsaP256 ? SigningKeyAlgorithm::EcdsaP256 : SigningKeyAlgorithm::EcdsaP384;
    const auto curve = NistPrimeCurve{curveName};
    // FIPS 186-5 section A.2.2: sample a full-width candidate and reject zero or values at least n.
    for (auto attempt = 0U; attempt < 128U; ++attempt) {
        auto scalar = core::application().secureRandom().buildByteBlock(unit::ByteLength{curve.byteLength()});
        scalar.markAsSensitive();
        const auto scalarEraseGuard = SecureEraseGuard{scalar};
        const auto number = curve.numberFromBigEndian(scalar.span());
        if (!number.has_value() || curve.isZero(*number) || curve.compare(*number, curve.order()) >= 0) {
            continue;
        }
        const auto point = ecdsa_signer::publicKey(scalar.span(), curveName);
        auto encoder = DerEncoder{};
        signing_key_encoding::appendEcPublicKey(encoder, point.span(), curveName);
        const auto publicKey = PublicKey::fromDerOrThrow(encoder.encoded());
        return SigningPrivateKey{algorithm, scalar.span(), publicKey};
    }
    throw CryptologyError{"Secure randomness did not produce a valid EC scalar within the fixed bound."_el};
}

}
