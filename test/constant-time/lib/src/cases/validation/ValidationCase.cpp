// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValidationCase.hpp"

#include "../../BackendInfo.hpp"

#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipherFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/GaloisMultiplierFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/ChaCha20BackendFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/Poly1305Factory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortablePoly1305.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

using namespace el::text::literals;

ValidationCase::ValidationCase(ValidationOperation operation) : _operation{operation} {
    const auto id = identifier();
    const auto keySize = operation == ValidationOperation::Aes128Gcm ? std::size_t{16} : std::size_t{32};
    const auto key = el::ByteBuffer{el::ByteLength{keySize}, el::Byte{0x55U}};
    const auto nonce = el::ByteBuffer{el::ByteLength{12}};
    auto backend = el::String{"portable"_el};
    if (operation == ValidationOperation::ChaCha20Poly1305) {
        const auto chacha = ci::createChaCha20Backend(key.span(), nonce.span());
        const auto poly = ci::createPoly1305(key.span());
        backend = el::StringFormat{"chacha20:{}+poly1305:{}"_el}.build(
            BackendInfo{*chacha, true}.name(),
            BackendInfo{*poly, BackendInfo::isImplementation<ci::PortablePoly1305>(*poly)}.name());
    } else if (operation < ValidationOperation::ChaCha20Poly1305) {
        backend = el::StringFormat{"aes:{}+ghash:{}"_el}.build(
            BackendInfo{
                *ci::createAesBlockCipher(key.span()), !ci::createAesBlockCipher(key.span())->isHardwareAccelerated()}
                .name(),
            BackendInfo{*ci::createGaloisMultiplier(), !ci::createGaloisMultiplier()->isHardwareAccelerated()}.name());
    } else {
        backend =
            BackendInfo{
                *ci::createAesBlockCipher(key.span()), !ci::createAesBlockCipher(key.span())->isHardwareAccelerated()}
                .name();
    }
    configure(
        TestMetadata{
            id,
            "Equivalent invalid-input rejection"_el,
            operation == ValidationOperation::CbcPadding ? "first-vs-last-invalid-padding-byte"_el
                                                         : "first-vs-last-invalid-tag-byte"_el,
            backend,
            true},
        el::ByteLength{16384});
}

auto ValidationCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<ValidationCase>(_operation);
}

auto ValidationCase::createFixture(el::Random &random, const bool population) -> std::unique_ptr<ValidationFixture> {
    return std::make_unique<ValidationFixture>(random, population, _operation);
}

auto ValidationCase::identifier() const -> el::String {
    switch (_operation) {
    case ValidationOperation::Aes128Gcm:
        return "aes-gcm/128/verify"_el;
    case ValidationOperation::Aes256Gcm:
        return "aes-gcm/256/verify"_el;
    case ValidationOperation::ChaCha20Poly1305:
        return "chacha20-poly1305/verify"_el;
    case ValidationOperation::CbcPadding:
        return "aes-cbc/invalid-padding"_el;
    }
    throw el::LogicError{"Unknown scenario operation."_el};
}

}
