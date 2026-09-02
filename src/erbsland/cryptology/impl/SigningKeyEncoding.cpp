// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SigningKeyEncoding.hpp"

#include "CryptologyOids.hpp"
#include "DerEncoder.hpp"

#include "../asn1/Asn1ObjectIdentifier.hpp"
#include "../keys/PublicKey.hpp"

#include "../../err/LogicError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology::impl::signing_key_encoding {

using namespace mem;
using namespace text::literals;

void appendEd25519PublicKey(DerEncoder &encoder, const ConstByteSpan point) {
    const auto root = encoder.beginSequence();
    const auto algorithm = encoder.beginSequence();
    encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::ed25519));
    encoder.end(algorithm);
    encoder.appendBitString(point);
    encoder.end(root);
}

void appendEcPublicKey(DerEncoder &encoder, const ConstByteSpan point, const NistPrimeCurve::Name curve) {
    const auto root = encoder.beginSequence();
    const auto algorithm = encoder.beginSequence();
    encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::ecPublicKey));
    encoder.appendObjectIdentifier(
        Asn1ObjectIdentifier::fromStringOrThrow(
            curve == NistPrimeCurve::Name::P256 ? cryptology_oids::secp256r1 : cryptology_oids::secp384r1));
    encoder.end(algorithm);
    encoder.appendBitString(point);
    encoder.end(root);
}

void appendPrivateKey(
    DerEncoder &encoder,
    const SigningKeyAlgorithm algorithm,
    const ConstByteSpan privateData,
    const PublicKey &publicKey) {
    encoder.markAsSensitive();
    const auto root = encoder.beginSequence();
    encoder.appendPositiveInteger(uint64_t{});
    switch (algorithm) {
    case SigningKeyAlgorithm::Ed25519: {
        const auto algorithmIdentifier = encoder.beginSequence();
        encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::ed25519));
        encoder.end(algorithmIdentifier);
        const auto privateKeyValue = encoder.beginOctetString();
        encoder.appendOctetString(privateData);
        encoder.end(privateKeyValue);
        break;
    }
    case SigningKeyAlgorithm::EcdsaP256:
    case SigningKeyAlgorithm::EcdsaP384: {
        const auto curve =
            algorithm == SigningKeyAlgorithm::EcdsaP256 ? NistPrimeCurve::Name::P256 : NistPrimeCurve::Name::P384;
        const auto algorithmIdentifier = encoder.beginSequence();
        encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::ecPublicKey));
        encoder.appendObjectIdentifier(
            Asn1ObjectIdentifier::fromStringOrThrow(
                curve == NistPrimeCurve::Name::P256 ? cryptology_oids::secp256r1 : cryptology_oids::secp384r1));
        encoder.end(algorithmIdentifier);
        const auto privateKeyValue = encoder.beginOctetString();
        const auto ecPrivateKey = encoder.beginSequence();
        encoder.appendPositiveInteger(uint64_t{1U});
        encoder.appendOctetString(privateData);
        const auto publicKeyValue = encoder.beginExplicit(1U);
        encoder.appendBitString(publicKey.keyData().span());
        encoder.end(publicKeyValue);
        encoder.end(ecPrivateKey);
        encoder.end(privateKeyValue);
        break;
    }
    case SigningKeyAlgorithm::Rsa:
        encoder.appendEncoded(publicKey.algorithm().toDer().span());
        encoder.appendOctetString(privateData);
        break;
    case SigningKeyAlgorithm::Unknown:
        throw err::LogicError{"Cannot encode an empty signing key."_el};
    }
    encoder.end(root);
}

}
