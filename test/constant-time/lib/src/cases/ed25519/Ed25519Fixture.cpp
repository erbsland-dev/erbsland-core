// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Ed25519Fixture.hpp"

#include <erbsland/cryptology/impl/algorithm/ed25519_signature/Ed25519Signer.hpp>
#include <erbsland/cryptology/impl/DerEncoder.hpp>
#include <erbsland/cryptology/impl/SigningKeyEncoding.hpp>
#include <erbsland/cryptology/keys/PublicKey.hpp>
#include <erbsland/cryptology/tls/TlsSignatureScheme.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

Ed25519Fixture::Ed25519Fixture(el::Random &random, const bool population, Ed25519Operation operation) :
    _operation{operation} {
    auto secret = input(random, population, 32);
    _secret = std::move(secret);
    _message = el::ByteBuffer{el::ByteLength{32}, el::Byte{0x33U}};
    _ed = ci::ed25519_signer::reduceScalar(_secret.span());
    const auto canonical = _ed.span();
    const auto field = ci::ed25519_signature::FieldElement::fromCanonicalBytes(canonical);
    require(field.has_value());
    _edField = *field;
    require(
        ci::ed25519_signature::FieldElement::equal(
            ci::ed25519_signature::FieldElement::multiply(_edField, ci::ed25519_signature::FieldElement::one()),
            _edField));
    if (operation <= Ed25519Operation::Sign) {
        const auto point = ci::ed25519_signer::publicKey(_secret.span());
        auto encoder = ci::DerEncoder{};
        ci::signing_key_encoding::appendEd25519PublicKey(encoder, point.span());
        const auto publicKey = el::PublicKey::fromDerOrThrow(encoder.encoded());
        require(publicKey.keyData().span().size() == 32);
        if (operation == Ed25519Operation::Sign) {
            const auto signature = ci::ed25519_signer::sign(_secret.span(), _message.span());
            require(publicKey.verifyTlsCertificateVerifySignature(
                el::TlsSignatureScheme::Ed25519, _message.span(), signature.span()));
        }
    }
}

template <Ed25519Operation tOperation>
auto Ed25519Fixture::sample() -> uint64_t {
    constexpr auto operation = tOperation;

    if constexpr (operation == Ed25519Operation::PublicKey) {
        return checksum(ci::ed25519_signer::publicKey(_secret.span()).span());
    } else if constexpr (operation == Ed25519Operation::Sign) {
        return checksum(ci::ed25519_signer::sign(_secret.span(), _message.span()).span());
    } else if constexpr (operation == Ed25519Operation::Reduce) {
        return checksum(ci::ed25519_signer::reduceScalar(_secret.span()).span());
    } else if constexpr (operation == Ed25519Operation::Add) {
        return checksum(ci::ed25519_signer::addScalars(_ed, _ed).span());
    } else if constexpr (operation == Ed25519Operation::Multiply) {
        return checksum(ci::ed25519_signer::multiplyScalars(_ed, _ed).span());
    } else if constexpr (operation == Ed25519Operation::BaseMultiply) {
        return checksum(ci::ed25519_signer::encodePoint(ci::ed25519_signer::multiplyBaseSecret(_ed.span())).span());
    } else if constexpr (operation == Ed25519Operation::FieldMultiply) {
        return checksum(ci::ed25519_signature::FieldElement::multiply(_edField, _edField).toBytes().span());
    } else {
        return checksum(ci::ed25519_signature::FieldElement::invert(_edField).toBytes().span());
    }
}

auto Ed25519Fixture::measure(const uint64_t repetitions) -> uint64_t {
    switch (_operation) {
    case Ed25519Operation::PublicKey:
        return repeat(repetitions, [this]() -> uint64_t { return sample<Ed25519Operation::PublicKey>(); });
    case Ed25519Operation::Sign:
        return repeat(repetitions, [this]() -> uint64_t { return sample<Ed25519Operation::Sign>(); });
    case Ed25519Operation::Reduce:
        return repeat(repetitions, [this]() -> uint64_t { return sample<Ed25519Operation::Reduce>(); });
    case Ed25519Operation::Add:
        return repeat(repetitions, [this]() -> uint64_t { return sample<Ed25519Operation::Add>(); });
    case Ed25519Operation::Multiply:
        return repeat(repetitions, [this]() -> uint64_t { return sample<Ed25519Operation::Multiply>(); });
    case Ed25519Operation::BaseMultiply:
        return repeat(repetitions, [this]() -> uint64_t { return sample<Ed25519Operation::BaseMultiply>(); });
    case Ed25519Operation::FieldMultiply:
        return repeat(repetitions, [this]() -> uint64_t { return sample<Ed25519Operation::FieldMultiply>(); });
    case Ed25519Operation::FieldInvert:
        return repeat(repetitions, [this]() -> uint64_t { return sample<Ed25519Operation::FieldInvert>(); });
    }
    throw el::LogicError{"Unknown sample operation."_el};
}

}
