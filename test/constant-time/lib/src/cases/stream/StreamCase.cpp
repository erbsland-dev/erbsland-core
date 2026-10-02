// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StreamCase.hpp"

#include "../../BackendInfo.hpp"

#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipherFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/GaloisMultiplierFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/ChaCha20BackendFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/Poly1305Factory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortableChaCha20Backend.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortablePoly1305.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

StreamCase::StreamCase(std::size_t size, StreamOperation operation, bool automatic) :
    _size{size}, _operation{operation}, _automatic{automatic} {

    const auto id = el::StringFormat{"{}/{}"_el}.build(operationName(), size);
    const auto key = el::ByteBuffer{el::ByteLength{32}, el::Byte{0x55U}};
    const auto nonce = el::ByteBuffer{el::ByteLength{12}};
    auto backend = el::String{"portable"_el};
    if (operation == StreamOperation::ChaCha20) {
        const auto worker = automatic ? ci::createChaCha20Backend(key.span(), nonce.span())
                                      : ci::createPortableChaCha20Backend(key.span(), nonce.span());
        backend =
            BackendInfo{*worker, size < 256 || BackendInfo::isImplementation<ci::PortableChaCha20Backend>(*worker)}
                .name();
    }
    if (operation == StreamOperation::Poly1305) {
        const auto worker = automatic ? ci::createPoly1305(key.span()) : ci::createPortablePoly1305(key.span());
        backend = BackendInfo{*worker, BackendInfo::isImplementation<ci::PortablePoly1305>(*worker)}.name();
    }
    if (operation == StreamOperation::AesGcm) {
        backend = el::StringFormat{"aes:{}+ghash:{}"_el}.build(
            BackendInfo{
                *ci::createAesBlockCipher(key.span()), !ci::createAesBlockCipher(key.span())->isHardwareAccelerated()}
                .name(),
            BackendInfo{*ci::createGaloisMultiplier(), !ci::createGaloisMultiplier()->isHardwareAccelerated()}.name());
    }
    if (operation == StreamOperation::ChaCha20Poly1305) {
        const auto chacha = ci::createChaCha20Backend(key.span(), nonce.span());
        const auto poly = ci::createPoly1305(key.span());
        backend = el::StringFormat{"chacha20:{}+poly1305:{}"_el}.build(
            BackendInfo{*chacha, size < 256 || BackendInfo::isImplementation<ci::PortableChaCha20Backend>(*chacha)}
                .name(),
            BackendInfo{*poly, BackendInfo::isImplementation<ci::PortablePoly1305>(*poly)}.name());
    }
    configure(
        TestMetadata{
            id, "Symmetric primitive and authenticated tag generation"_el, "fixed-vs-random-key"_el, backend, true},
        el::ByteLength{8192 + size * 2});
}

auto StreamCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<StreamCase>(_size, _operation, automatic);
}

auto StreamCase::createFixture(el::Random &random, const bool population) -> std::unique_ptr<StreamFixture> {
    return std::make_unique<StreamFixture>(random, population, _size, _operation, _automatic);
}

auto StreamCase::operationName() const -> el::String {
    switch (_operation) {
    case StreamOperation::ChaCha20:
        return "chacha20"_el;
    case StreamOperation::Poly1305:
        return "poly1305"_el;
    case StreamOperation::AesGcm:
        return "aes-gcm/tag"_el;
    case StreamOperation::ChaCha20Poly1305:
        return "chacha20-poly1305/tag"_el;
    }
    throw el::LogicError{"Unknown scenario operation."_el};
}

}
