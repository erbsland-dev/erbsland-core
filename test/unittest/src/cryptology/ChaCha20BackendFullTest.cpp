// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/ChaCha20BackendFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/Poly1305Factory.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/SymmetricEncryptor.hpp>

#include <algorithm>
#include <array>

using namespace el::cryptology;

TESTED_TARGETS(ChaCha20Backend Poly1305 ArmChaCha20Backend X86ChaCha20Backend ArmPoly1305 X86Poly1305)
class ChaCha20BackendFullTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    [[nodiscard]] static auto deterministicBytes(const std::size_t length, const uint8_t seed) -> el::mem::ByteBlock {
        auto result = el::mem::ByteBlockEditor{el::unit::ByteCount::fromSizeT(length)};
        for (
            auto index = el::unit::ByteIndex::zero(); index.isWithin(el::unit::ByteCount::fromSizeT(length));
            index.uncheckedIncrement()) {
            result.set(
                index,
                el::mem::Byte{static_cast<uint8_t>(seed + index.toRawValue() * 29U + (index.toRawValue() >> 2U))});
        }
        return result;
    }

    struct Encrypted final {
        el::mem::ByteBlock ciphertext;
        SymmetricTag tag;
    };

    [[nodiscard]] static auto encryptWithPolicy(const bool accelerated, const el::mem::ByteBlock &plaintext)
        -> Encrypted {
        el::core::application().cryptologyConfiguration().setHardwareAccelerationEnabled(accelerated);
        const auto key = SymmetricKey{deterministicBytes(32U, 0x21U)};
        const auto nonce = SymmetricNonce{deterministicBytes(12U, 0x43U)};
        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::ChaCha20Poly1305, key, nonce};
        encryptor.addAuthenticatedData(deterministicBytes(37U, 0x65U));
        auto ciphertext = el::mem::ByteBlockEditor{};
        for (auto offset = std::size_t{}; offset < plaintext.length().toSizeT();) {
            const auto count = std::min((offset * 7U) % 83U + 1U, plaintext.length().toSizeT() - offset);
            ciphertext.append(encryptor.encrypt(plaintext.span().subspan(offset, count)));
            offset += count;
        }
        ciphertext.append(encryptor.finalize());
        return {ciphertext, encryptor.tag()};
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testPortableAndSelectedChaCha20Blocks() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto key = deterministicBytes(32U, 0x17U);
        const auto nonce = deterministicBytes(12U, 0x31U);
        for (
            const auto offset :
            {el::unit::ByteCount{0U},
                el::unit::ByteCount{1U},
                el::unit::ByteCount{3U},
                el::unit::ByteCount{7U},
                el::unit::ByteCount{15U}}) {
            auto keyStorage = el::mem::ByteBlockEditor{el::unit::ByteCount{32U} + offset};
            auto nonceStorage = el::mem::ByteBlockEditor{el::unit::ByteCount{12U} + offset};
            keyStorage.overwrite(el::unit::ByteIndex::end(offset), key.span());
            nonceStorage.overwrite(el::unit::ByteIndex::end(offset), nonce.span());
            const auto keySpan = keyStorage.span(el::unit::ByteIndex::end(offset), el::unit::ByteCount{32U});
            const auto nonceSpan = nonceStorage.span(el::unit::ByteIndex::end(offset), el::unit::ByteCount{12U});
            auto portable = el::cryptology::impl::createPortableChaCha20Backend(keySpan, nonceSpan);
            auto selected = el::cryptology::impl::createChaCha20Backend(keySpan, nonceSpan);
            for (const auto counter : {uint32_t{1U}, uint32_t{7U}, uint32_t{0xfffffffcU}}) {
                const auto expected = portable->generateBlocks(counter, 4U);
                const auto actual = selected->generateBlocks(counter, 4U);
                REQUIRE_EQUAL(expected, actual);
            }
        }
    }

    void testPortableAndSelectedPoly1305Streaming() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto key = deterministicBytes(32U, 0x51U);
        for (
            const auto alignment :
            {el::unit::ByteCount{0U},
                el::unit::ByteCount{1U},
                el::unit::ByteCount{3U},
                el::unit::ByteCount{7U},
                el::unit::ByteCount{15U}}) {
            auto keyStorage = el::mem::ByteBlockEditor{el::unit::ByteCount{32U} + alignment};
            keyStorage.overwrite(el::unit::ByteIndex::end(alignment), key.span());
            const auto keySpan = keyStorage.span(el::unit::ByteIndex::end(alignment), el::unit::ByteCount{32U});
            for (const auto length : {0U, 1U, 15U, 16U, 17U, 63U, 64U, 65U, 255U, 256U, 257U, 1024U}) {
                const auto message = deterministicBytes(length, 0x73U);
                auto portable = el::cryptology::impl::createPortablePoly1305(keySpan);
                auto selected = el::cryptology::impl::createPoly1305(keySpan);
                for (auto offset = std::size_t{}; offset < length;) {
                    const auto count = std::min((offset * 11U) % 47U + 1U, length - offset);
                    portable->update(message.span().subspan(offset, count));
                    selected->update(message.span().subspan(offset, count));
                    offset += count;
                }
                REQUIRE_EQUAL(selected->finalize(), portable->finalize());
            }
        }
    }

    void testPortableAndSelectedCompleteAead() {
        const auto applicationScope = ApplicationTestScope<>{};
        for (const auto length : {0U, 1U, 15U, 16U, 17U, 63U, 64U, 65U, 255U, 256U, 257U, 1024U}) {
            const auto plaintext = deterministicBytes(length, 0x95U);
            const auto portable = encryptWithPolicy(false, plaintext);
            const auto accelerated = encryptWithPolicy(true, plaintext);
            REQUIRE_EQUAL(accelerated.ciphertext, portable.ciphertext);
            REQUIRE_EQUAL(accelerated.tag.data(), portable.tag.data());
        }
    }
};
