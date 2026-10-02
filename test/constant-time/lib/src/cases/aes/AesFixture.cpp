// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AesFixture.hpp"

#include <erbsland/cryptology/impl/algorithm/aes/AesKeySchedule.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>

namespace app::constant_time {

using namespace el::text::literals;

AesFixture::AesFixture(
    el::Random &random, const bool population, std::size_t keySize, AesOperation operation, bool automatic) :
    _operation{operation} {

    auto key = input(random, population, keySize);
    auto cipher = automatic ? ci::createAesBlockCipher(key.span()) : ci::createPortableAesBlockCipher(key.span());
    const auto plaintext = ci::AesBlockCipher::Block{};
    const auto ciphertext = cipher->encrypt(plaintext);
    require(cipher->decrypt(ciphertext) == plaintext);
    require(ci::createPortableAesBlockCipher(key.span())->encrypt(plaintext) == ciphertext);
    _key = std::move(key);
    _cipher = std::move(cipher);
    _plaintext = plaintext;
    _ciphertext = ciphertext;
}

template <AesOperation tOperation>
auto AesFixture::sample() -> uint64_t {
    constexpr auto operation = tOperation;

    if constexpr (operation == AesOperation::KeyExpansion) {
        const auto schedule = ci::AesKeySchedule{_key.span()};
        return checksum(schedule.roundKey(0).span());
    } else {
        const auto output =
            operation == AesOperation::Encrypt ? _cipher->encrypt(_plaintext) : _cipher->decrypt(_ciphertext);
        return checksum(output.span());
    }
}

auto AesFixture::measure(const uint64_t repetitions) -> uint64_t {
    switch (_operation) {
    case AesOperation::KeyExpansion:
        return repeat(repetitions, [this]() -> uint64_t { return sample<AesOperation::KeyExpansion>(); });
    case AesOperation::Encrypt:
        return repeat(repetitions, [this]() -> uint64_t { return sample<AesOperation::Encrypt>(); });
    case AesOperation::Decrypt:
        return repeat(repetitions, [this]() -> uint64_t { return sample<AesOperation::Decrypt>(); });
    }
    throw el::LogicError{"Unknown sample operation."_el};
}

}
