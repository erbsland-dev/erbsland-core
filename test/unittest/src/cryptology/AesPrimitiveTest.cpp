// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/AesBlockCipherFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/AesKeySchedule.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/AesOperations.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/GaloisMultiplierFactory.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/PortableAesBlockCipher.hpp>
#include <erbsland/cryptology/impl/algorithm/aes/PortableGaloisMultiplier.hpp>

using el::cryptology::impl::AesBlockCipher;
using el::cryptology::impl::AesKeySchedule;
using el::cryptology::impl::PortableAesBlockCipher;
using el::cryptology::impl::PortableGaloisMultiplier;
using el::mem::ByteArray;

TESTED_TARGETS(AesBlockCipher AesKeySchedule PortableAesBlockCipher PortableGaloisMultiplier)
class AesPrimitiveTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    template <std::size_t N>
    static auto arrayFromHex(const std::string_view text) -> ByteArray<N> {
        return ByteArray<N>::fromSpanOrThrow(bytesFromHex(text).span());
    }

public:
    void testSubstitution() {
        REQUIRE_EQUAL(el::cryptology::impl::aes::substitute(0x53U), el::mem::Byte{0xedU});
        REQUIRE_EQUAL(el::cryptology::impl::aes::inverseSubstitute(0xedU), el::mem::Byte{0x53U});
        for (auto value = 0U; value < 256U; ++value) {
            const auto byte = el::mem::Byte{static_cast<uint8_t>(value)};
            REQUIRE_EQUAL(
                el::cryptology::impl::aes::inverseSubstitute(el::cryptology::impl::aes::substitute(byte)), byte);
        }
    }

    void testAes128() {
        const auto key = arrayFromHex<16>("000102030405060708090a0b0c0d0e0f");
        const auto input = arrayFromHex<16>("00112233445566778899aabbccddeeff");
        const auto expected = arrayFromHex<16>("69c4e0d86a7b0430d8cdb78070b4c55a");
        auto cipher = PortableAesBlockCipher{key.span()};
        REQUIRE_EQUAL(cipher.encrypt(input), expected);
        REQUIRE_EQUAL(cipher.decrypt(expected), input);

        // FIPS 197, Appendix A.1: first and final AES-128 expanded round keys.
        const auto schedule = AesKeySchedule{key.span()};
        REQUIRE_EQUAL(schedule.roundKey(1U), arrayFromHex<16>("d6aa74fdd2af72fadaa678f1d6ab76fe"));
        REQUIRE_EQUAL(schedule.roundKey(10U), arrayFromHex<16>("13111d7fe3944a17f307a78b4d2b30c5"));
    }

    void testAes256() {
        const auto key = arrayFromHex<32>("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
        const auto input = arrayFromHex<16>("00112233445566778899aabbccddeeff");
        const auto expected = arrayFromHex<16>("8ea2b7ca516745bfeafc49904b496089");
        auto cipher = PortableAesBlockCipher{key.span()};
        REQUIRE_EQUAL(cipher.encrypt(input), expected);
        REQUIRE_EQUAL(cipher.decrypt(expected), input);

        // FIPS 197, Appendix A.3: second and final AES-256 expanded round keys.
        const auto schedule = AesKeySchedule{key.span()};
        REQUIRE_EQUAL(schedule.roundKey(2U), arrayFromHex<16>("a573c29fa176c498a97fce93a572c09c"));
        REQUIRE_EQUAL(schedule.roundKey(14U), arrayFromHex<16>("24fc79ccbf0979e9371ac23c6d68de36"));
    }

    void testGaloisMultiply() {
        // NIST SP 800-38D, Test Case 2: GHASH multiplication used after the single ciphertext block.
        const auto left = arrayFromHex<16>("0388dace60b6a392f328c2b971b2fe78");
        const auto right = arrayFromHex<16>("66e94bd4ef8a2c3b884cfa59ca342b2e");
        const auto expected = arrayFromHex<16>("5e2ec746917062882c85b0685353deb7");
        const auto multiplier = PortableGaloisMultiplier{};
        REQUIRE_EQUAL(multiplier.multiply(left, right), expected);
    }

    void testSelectedBackends() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto key = arrayFromHex<16>("000102030405060708090a0b0c0d0e0f");
        const auto input = arrayFromHex<16>("00112233445566778899aabbccddeeff");
        const auto expected = arrayFromHex<16>("69c4e0d86a7b0430d8cdb78070b4c55a");
        auto cipher = el::cryptology::impl::createAesBlockCipher(key.span());
        REQUIRE_EQUAL(cipher->encrypt(input), expected);
        REQUIRE_EQUAL(cipher->decrypt(expected), input);

        const auto left = arrayFromHex<16>("0388dace60b6a392f328c2b971b2fe78");
        const auto right = arrayFromHex<16>("66e94bd4ef8a2c3b884cfa59ca342b2e");
        const auto galoisExpected = arrayFromHex<16>("5e2ec746917062882c85b0685353deb7");
        const auto multiplier = el::cryptology::impl::createGaloisMultiplier();
        const auto actual = multiplier->multiply(left, right);
        for (auto index = el::unit::ByteIndex{}; index < actual.endIndex(); ++index) {
            REQUIRE_EQUAL(actual.get(index), galoisExpected.get(index));
        }

        // Exercise deterministic inputs against the selected AES and GHASH backends to detect backend drift.
        auto portableCipher = PortableAesBlockCipher{key.span()};
        const auto portableMultiplier = PortableGaloisMultiplier{};
        for (auto iteration = uint8_t{}; iteration < 64U; ++iteration) {
            auto block = AesBlockCipher::Block{};
            auto factor = AesBlockCipher::Block{};
            for (auto index = std::size_t{}; index < 16U; ++index) {
                block.set(el::unit::ByteIndex{index}, el::mem::Byte{static_cast<uint8_t>(iteration + index * 17U)});
                factor.set(
                    el::unit::ByteIndex{index}, el::mem::Byte{static_cast<uint8_t>(iteration * 3U + index * 29U)});
            }
            REQUIRE_EQUAL(cipher->encrypt(block), portableCipher.encrypt(block));
            REQUIRE_EQUAL(cipher->decrypt(block), portableCipher.decrypt(block));
            REQUIRE_EQUAL(multiplier->multiply(block, factor), portableMultiplier.multiply(block, factor));
        }
    }
};
