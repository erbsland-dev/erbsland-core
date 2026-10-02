// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValidationFixture.hpp"

#include <erbsland/cryptology/CryptologyError.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipherFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/Poly1305Factory.hpp>
#include <erbsland/cryptology/impl/symmetric/AesCbcState.hpp>
#include <erbsland/cryptology/impl/symmetric/AesGcmDecryptorData.hpp>
#include <erbsland/cryptology/impl/symmetric/AesGcmState.hpp>
#include <erbsland/cryptology/impl/symmetric/ChaCha20Poly1305DecryptorData.hpp>
#include <erbsland/cryptology/impl/symmetric/ChaCha20Poly1305State.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptionType.hpp>
#include <erbsland/cryptology/symmetric/SymmetricTag.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

namespace ci = erbsland::cryptology::impl;

using namespace el::text::literals;

ValidationFixture::ValidationFixture(
    [[maybe_unused]] el::Random &random, const bool population, ValidationOperation operation) :
    _operation{operation} {
    const auto keySize = operation == ValidationOperation::Aes128Gcm ? 16U : 32U;
    _key = el::ByteBuffer{el::ByteLength{keySize}, el::Byte{0x55U}};
    _nonce = el::ByteBuffer{el::ByteLength{operation == ValidationOperation::CbcPadding ? 16U : 12U}};
    auto data = el::ByteBuffer{el::ByteLength{32}, el::Byte{0x33U}};
    auto tag = el::ByteBuffer{};
    if (operation < ValidationOperation::ChaCha20Poly1305) {
        auto state = ci::AesGcmState{_key.span(), _nonce.span()};
        _ciphertext = state.transform(data.span(), true);
        tag = el::ByteBuffer{state.finalizeTag().span()};
    } else if (operation == ValidationOperation::ChaCha20Poly1305) {
        auto state = ci::ChaCha20Poly1305State{_key.span(), _nonce.span()};
        _ciphertext = state.transform(data.span(), true);
        tag = el::ByteBuffer{state.finalizeTag().span()};
    } else {
        auto plaintext = ci::AesBlockCipher::Block{};
        // No marker in either population; vary the position of the same invalid nonzero byte.
        plaintext.set(el::ByteIndex{population ? 15U : 0U}, el::Byte{0x01U});
        _ciphertext = el::ByteBlock::fromSpan(ci::createAesBlockCipher(_key.span())->encrypt(plaintext).span());
    }
    if (!tag.isEmpty()) {
        const auto index = population ? 15U : 0U;
        tag.set(el::ByteIndex{index}, tag.get(el::ByteIndex{index}) ^ el::Byte{1U});
        _tag = el::SymmetricTag{tag.span()};
    }
    require(measure(1) == 1);
}

template <ValidationOperation tOperation>
auto ValidationFixture::reject() -> uint64_t {
    constexpr auto operation = tOperation;

    try {
        if constexpr (operation == ValidationOperation::Aes128Gcm || operation == ValidationOperation::Aes256Gcm) {
            auto state = ci::AesGcmDecryptorData{
                operation == ValidationOperation::Aes128Gcm ? el::SymmetricEncryptionType::Aes128Gcm
                                                            : el::SymmetricEncryptionType::Aes256Gcm,
                _key.span(),
                _nonce.span()};
            const auto output = state.decrypt(_ciphertext.span());
            const auto final = state.finalize(_tag);
            Checksum::instance().retain(checksum(output.span()) + checksum(final.span()));
            return 0;
        } else if constexpr (operation == ValidationOperation::ChaCha20Poly1305) {
            auto state = ci::ChaCha20Poly1305DecryptorData{_key.span(), _nonce.span()};
            const auto output = state.decrypt(_ciphertext.span());
            const auto final = state.finalize(_tag);
            Checksum::instance().retain(checksum(output.span()) + checksum(final.span()));
            return 0;
        } else {
            auto state =
                ci::AesCbcState{el::SymmetricEncryptionType::Aes256CbcIso9797Method2, _key.span(), _nonce.span()};
            const auto output = state.decrypt(_ciphertext.span());
            const auto final = state.finalizeDecryption();
            Checksum::instance().retain(checksum(output.span()) + checksum(final.span()));
            return 0;
        }
    } catch (const el::CryptologyError &) {
        return 1;
    }
}

template <ValidationOperation tOperation>
auto ValidationFixture::sample() -> uint64_t {
    return reject<tOperation>();
}

auto ValidationFixture::measure(const uint64_t repetitions) -> uint64_t {
    switch (_operation) {
    case ValidationOperation::Aes128Gcm:
        return repeat(repetitions, [this]() -> uint64_t { return sample<ValidationOperation::Aes128Gcm>(); });
    case ValidationOperation::Aes256Gcm:
        return repeat(repetitions, [this]() -> uint64_t { return sample<ValidationOperation::Aes256Gcm>(); });
    case ValidationOperation::ChaCha20Poly1305:
        return repeat(repetitions, [this]() -> uint64_t { return sample<ValidationOperation::ChaCha20Poly1305>(); });
    case ValidationOperation::CbcPadding:
        return repeat(repetitions, [this]() -> uint64_t { return sample<ValidationOperation::CbcPadding>(); });
    }
    throw el::LogicError{"Unknown sample operation."_el};
}

}
