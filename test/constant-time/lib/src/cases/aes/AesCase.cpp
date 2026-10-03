// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AesCase.hpp"

#include "../../BackendInfo.hpp"

#include <erbsland/Byte.hpp>
#include <erbsland/ByteBuffer.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipherFactory.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/Random.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringFormat.hpp>

namespace app::constant_time {

using namespace el::text::literals;

AesCase::AesCase(el::ByteCount keySize, AesOperation operation, bool automatic) :
    _keySize{keySize}, _operation{operation}, _automatic{automatic} {
    const auto id = el::StringFormat{"aes/{}/{}"_el}.build(keySize.toRawValue() * 8, operationName());
    const auto key = el::ByteBuffer{keySize, el::Byte{0x55U}};
    const auto probe = automatic ? ci::createAesBlockCipher(key.span()) : ci::createPortableAesBlockCipher(key.span());
    configure(
        TestMetadata{
            id,
            "AES secret-key operation"_el,
            "fixed-vs-random-key"_el,
            BackendInfo{*probe, operation == AesOperation::KeyExpansion || !probe->isHardwareAccelerated()}.name(),
            operation != AesOperation::KeyExpansion},
        el::ByteCount{4096});
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
    throw el::LogicFailure{"Unknown scenario operation."_el};
}

}
