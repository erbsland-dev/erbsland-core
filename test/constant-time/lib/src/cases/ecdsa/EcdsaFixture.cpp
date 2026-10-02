// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EcdsaFixture.hpp"

#include <erbsland/cryptology/impl/algorithm/ecdsa_signature/EcdsaSigner.hpp>
#include <erbsland/cryptology/impl/SigningKeyEncoding.hpp>
#include <erbsland/cryptology/keys/PublicKey.hpp>
#include <erbsland/cryptology/tls/TlsSignatureScheme.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

EcdsaFixture::EcdsaFixture(
    el::Random &random, const bool population, ci::NistPrimeCurve::Name name, EcdsaOperation operation) :
    _name{name}, _operation{operation} {
    const auto curve = ci::NistPrimeCurve{name};
    auto secret = input(random, population, curve.byteLength());
    // Both classes use valid nonzero scalars below the curve order with identical public
    // width.
    secret.set(el::ByteIndex{}, el::Byte{1U});
    _secret = std::move(secret);
    _message = el::ByteBuffer{el::ByteLength{32}, el::Byte{0x33U}};
    _nist = ci::ecdsa_signer::decodePrivateScalar(_secret.span(), name);
    require(curve.compare(_nist, curve.order()) < 0 && !curve.isZero(_nist));
    if (operation <= EcdsaOperation::Sign) {
        const auto point = ci::ecdsa_signer::publicKey(_secret.span(), name);
        auto encoder = ci::DerEncoder{};
        ci::signing_key_encoding::appendEcPublicKey(encoder, point.span(), name);
        const auto publicKey = el::PublicKey::fromDerOrThrow(encoder.encoded());
        if (operation == EcdsaOperation::Sign) {
            const auto signature = ci::ecdsa_signer::sign(_secret.span(), _message.span(), name);
            require(publicKey.verifyTlsCertificateVerifySignature(
                name == ci::NistPrimeCurve::Name::P256 ? el::TlsSignatureScheme::EcdsaSecp256r1Sha256
                                                       : el::TlsSignatureScheme::EcdsaSecp384r1Sha384,
                _message.span(),
                signature.span()));
        }
    }
}

template <EcdsaOperation tOperation>
auto EcdsaFixture::sample() -> uint64_t {
    constexpr auto operation = tOperation;
    const auto name = _name;
    const auto curve = ci::NistPrimeCurve{name};
    if constexpr (operation == EcdsaOperation::PublicKey) {
        return checksum(ci::ecdsa_signer::publicKey(_secret.span(), name).span());
    } else if constexpr (operation == EcdsaOperation::Sign) {
        return checksum(ci::ecdsa_signer::sign(_secret.span(), _message.span(), name).span());
    } else if constexpr (operation == EcdsaOperation::Multiply) {
        return checksum(curve.numberToBigEndian(curve.multiplyOrderSecret(_nist, _nist)).span());
    } else if constexpr (operation == EcdsaOperation::Invert) {
        return checksum(curve.numberToBigEndian(curve.invertOrderSecret(_nist)).span());
    } else if constexpr (operation == EcdsaOperation::Add) {
        return checksum(curve.numberToBigEndian(curve.addOrderSecret(_nist, _nist)).span());
    } else {
        return checksum(curve.numberToBigEndian(curve.multiplyBaseSecret(_nist).x).span());
    }
}

auto EcdsaFixture::measure(const uint64_t repetitions) -> uint64_t {
    switch (_operation) {
    case EcdsaOperation::PublicKey:
        return repeat(repetitions, [this]() -> uint64_t { return sample<EcdsaOperation::PublicKey>(); });
    case EcdsaOperation::Sign:
        return repeat(repetitions, [this]() -> uint64_t { return sample<EcdsaOperation::Sign>(); });
    case EcdsaOperation::Multiply:
        return repeat(repetitions, [this]() -> uint64_t { return sample<EcdsaOperation::Multiply>(); });
    case EcdsaOperation::Invert:
        return repeat(repetitions, [this]() -> uint64_t { return sample<EcdsaOperation::Invert>(); });
    case EcdsaOperation::Add:
        return repeat(repetitions, [this]() -> uint64_t { return sample<EcdsaOperation::Add>(); });
    case EcdsaOperation::BaseMultiply:
        return repeat(repetitions, [this]() -> uint64_t { return sample<EcdsaOperation::BaseMultiply>(); });
    }
    throw el::LogicError{"Unknown sample operation."_el};
}

}
