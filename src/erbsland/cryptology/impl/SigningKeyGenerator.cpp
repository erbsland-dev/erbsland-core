// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SigningKeyGenerator.hpp"

#include "CryptologyOids.hpp"
#include "DerEncoder.hpp"
#include "EcPrivateScalarPolicy.hpp"
#include "SecureEraseGuard.hpp"
#include "SigningKeyEncoding.hpp"

#include "algorithm/ecdsa_signature/EcdsaSigner.hpp"
#include "algorithm/ed25519_signature/Ed25519Signer.hpp"
#include "algorithm/rsa_signature/RsaKeyGenerator.hpp"
#include "algorithm/rsa_signature/RsaSigner.hpp"

#include "../CryptologyError.hpp"
#include "../keys/PublicKey.hpp"
#include "../keys/SigningPrivateKey.hpp"

#include "../../core/impl/application_data/ApplicationRandomData.hpp"
#include "../../core/impl/ApplicationDataAccess.hpp"
#include "../../random/Random.hpp"
#include "../../random/RandomFailure.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringFormat.hpp"
#include "../../unit/ByteCount.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

auto SigningKeyGenerator::generate() const -> SigningPrivateKey {
    switch (_profile) {
    case SigningKeyProfile::Ed25519:
        return generateEd25519();
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

auto SigningKeyGenerator::generateEd25519() const -> SigningPrivateKey {
    const auto randomData = core::impl::applicationData()->random().get();
    auto seed = randomData->secureRandom().buildByteBlock(unit::ByteCount{32U});
    seed.markAsSensitive();
    const auto seedEraseGuard = SecureEraseGuard{seed};
    const auto point = ed25519_signer::publicKey(seed.span());
    auto encoder = DerEncoder{};
    signing_key_encoding::appendEd25519PublicKey(encoder, point.span());
    auto publicKey = PublicKey::fromDerOrThrow(encoder.encoded());
    return SigningPrivateKey{SigningKeyAlgorithm::Ed25519, seed.span(), std::move(publicKey)};
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
    // Use full-width rejection sampling with the additional library policy for extreme signing scalars.
    const auto randomData = core::impl::applicationData()->random().get();
    for (auto attempt = 0U; attempt < cMaximumEcAttempts; ++attempt) {
        auto scalar = randomData->secureRandom().buildByteBlock(curve.byteLength());
        scalar.markAsSensitive(); // should be already marked it as sensitive, but better safe than sorry.
        const auto scalarEraseGuard = SecureEraseGuard{scalar};
        if (!ec_private_scalar_policy::isAcceptable(scalar.span(), curveName)) {
            continue;
        }
        const auto point = ecdsa_signer::publicKey(scalar.span(), curveName);
        auto encoder = DerEncoder{};
        signing_key_encoding::appendEcPublicKey(encoder, point.span(), curveName);
        const auto publicKey = PublicKey::fromDerOrThrow(encoder.encoded());
        return SigningPrivateKey{algorithm, scalar.span(), publicKey};
    }
    throw random::RandomFailure{
        text::StringFormat{"EC P-{} signing-key generation rejected all {} random candidates."_el}.build(
            curve.byteLength().toSizeT() * 8U, cMaximumEcAttempts)};
}

}
