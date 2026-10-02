// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AesCase.hpp"

#include "../../BackendInfo.hpp"

#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipherFactory.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

AesCase::AesCase(std::size_t keySize, AesOperation operation, bool automatic) :
    _keySize{keySize}, _operation{operation}, _automatic{automatic} {
    const auto id = el::StringFormat{"aes/{}/{}"_el}.build(keySize * 8, operationName());
    const auto key = el::ByteBuffer{el::ByteLength{keySize}, el::Byte{0x55U}};
    const auto probe = automatic ? ci::createAesBlockCipher(key.span()) : ci::createPortableAesBlockCipher(key.span());
    configure(
        TestMetadata{
            id,
            "AES secret-key operation"_el,
            "fixed-vs-random-key"_el,
            BackendInfo{*probe, operation == AesOperation::KeyExpansion || !probe->isHardwareAccelerated()}.name(),
            operation != AesOperation::KeyExpansion},
        el::ByteLength{4096});
}

auto AesCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<AesCase>(_keySize, _operation, automatic);
}

auto AesCase::createFixture(el::Random &random, const bool population) -> std::unique_ptr<AesFixture> {
    return std::make_unique<AesFixture>(random, population, _keySize, _operation, _automatic);
}

auto AesCase::operationName() const -> el::String {
    switch (_operation) {
    case AesOperation::KeyExpansion:
        return "key-expansion"_el;
    case AesOperation::Encrypt:
        return "encrypt"_el;
    case AesOperation::Decrypt:
        return "decrypt"_el;
    }
    throw el::LogicError{"Unknown scenario operation."_el};
}

}
