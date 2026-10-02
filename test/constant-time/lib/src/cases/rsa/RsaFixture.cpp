// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RsaFixture.hpp"

#include <erbsland/cryptology/impl/algorithm/rsa_signature/RsaSigner.hpp>
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

RsaFixture::RsaFixture(
    el::Random &random, const bool population, std::shared_ptr<RsaContext> context, RsaOperation operation) :
    _context{context}, _operation{operation} {
    auto bytes = input(random, population, 256);
    bytes.set(el::ByteIndex{}, el::Byte{});
    auto exponent = input(random, population, 256);
    _value = ci::rsa_signature::Number::fromBigEndian(bytes.span(), 64);
    _exponent = ci::rsa_signature::Number::fromBigEndian(exponent.span(), 64);
    _message = input(random, population, 32);
    _salt = input(random, population, 48);
    _blinding = el::ByteBuffer{el::ByteLength{256}};
    _blinding.set(el::ByteIndex{255}, el::Byte{2U});
    require(_value.compare(context->components().modulus) < 0);
    if (operation >= RsaOperation::SignSha256) {
        const auto scheme = operation == RsaOperation::SignSha256 ? el::TlsSignatureScheme::RsaPssRsaeSha256
                                                                  : el::TlsSignatureScheme::RsaPssRsaeSha384;
        const auto signature = ci::rsa_signer::signWithRandom(
            context->privateKey().span(),
            scheme,
            _message.span(),
            context->publicKey(),
            _salt.span().first(operation == RsaOperation::SignSha256 ? 32 : 48),
            _blinding.span());
        require(context->publicKey().verifyTlsCertificateVerifySignature(scheme, _message.span(), signature.span()));
    } else {
        require(_value.reducedSecret(context->components().modulus).isEqual(_value, 64));
    }
}

template <RsaOperation tOperation>
auto RsaFixture::sample() -> uint64_t {
    constexpr auto operation = tOperation;
    const auto &context = _context;
    const auto &modulus = context->components().modulus;
    if constexpr (operation >= RsaOperation::SignSha256) {
        const auto signature = ci::rsa_signer::signWithRandom(
            context->privateKey().span(),
            operation == RsaOperation::SignSha256 ? el::TlsSignatureScheme::RsaPssRsaeSha256
                                                  : el::TlsSignatureScheme::RsaPssRsaeSha384,
            _message.span(),
            context->publicKey(),
            _salt.span().first(operation == RsaOperation::SignSha256 ? 32 : 48),
            _blinding.span());
        return checksum(signature.span());
    } else {
        const auto output = operation == RsaOperation::Reduce ? _value.reducedSecret(modulus)
            : operation == RsaOperation::Multiply             ? _value.multipliedModuloSecret(_value, modulus)
            : operation == RsaOperation::Subtract
            ? _value.subtractedModuloSecret(_exponent.reducedSecret(modulus), modulus)
            : _value.poweredModuloSecret(_exponent, modulus);
        return output.word(0);
    }
}

auto RsaFixture::measure(const uint64_t repetitions) -> uint64_t {
    switch (_operation) {
    case RsaOperation::Reduce:
        return repeat(repetitions, [this]() -> uint64_t { return sample<RsaOperation::Reduce>(); });
    case RsaOperation::Multiply:
        return repeat(repetitions, [this]() -> uint64_t { return sample<RsaOperation::Multiply>(); });
    case RsaOperation::Subtract:
        return repeat(repetitions, [this]() -> uint64_t { return sample<RsaOperation::Subtract>(); });
    case RsaOperation::Power:
        return repeat(repetitions, [this]() -> uint64_t { return sample<RsaOperation::Power>(); });
    case RsaOperation::SignSha256:
        return repeat(repetitions, [this]() -> uint64_t { return sample<RsaOperation::SignSha256>(); });
    case RsaOperation::SignSha384:
        return repeat(repetitions, [this]() -> uint64_t { return sample<RsaOperation::SignSha384>(); });
    }
    throw el::LogicError{"Unknown sample operation."_el};
}

}
