// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/impl/algorithm/chacha20/ChaCha20Operations.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortableChaCha20Backend.hpp>
#include <erbsland/cryptology/impl/algorithm/chacha20/PortablePoly1305.hpp>
#include <erbsland/mem/ByteArray.hpp>

#include <array>

using el::cryptology::impl::PortableChaCha20Backend;
using el::cryptology::impl::PortablePoly1305;
using el::mem::ByteArray;

TESTED_TARGETS(ChaCha20Operations ChaCha20Backend PortableChaCha20Backend Poly1305 PortablePoly1305)
class ChaCha20PrimitiveTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    template <std::size_t N>
    [[nodiscard]] static auto arrayFromHex(const std::string_view text) -> ByteArray<N> {
        return ByteArray<N>::fromSpanOrThrow(bytesFromHex(text).span());
    }

public:
    void testQuarterRoundRfc8439Section211() {
        auto state = el::cryptology::impl::chacha20::State{};
        state[0] = 0x11111111U;
        state[1] = 0x01020304U;
        state[2] = 0x9b8d6f43U;
        state[3] = 0x01234567U;

        // RFC 8439, Section 2.1.1: direct quarter-round known-answer example.
        el::cryptology::impl::chacha20::quarterRound(state, 0U, 1U, 2U, 3U);
        REQUIRE_EQUAL(state[0], 0xea2a92f4U);
        REQUIRE_EQUAL(state[1], 0xcb1cf8ceU);
        REQUIRE_EQUAL(state[2], 0x4581472eU);
        REQUIRE_EQUAL(state[3], 0x5881c4bbU);
    }

    void testBlockRfc8439Section231() {
        const auto key = arrayFromHex<32>("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
        const auto nonce = arrayFromHex<12>("000000090000004a00000000");
        const auto expected = arrayFromHex<64>("10f1e7e4d13b5915500fdd1fa32071c4c7d1f4c733c068030422aa9ac3d46c4e"
                                               "d2826446079faa0914c2d705d98b02a2b5129cd1de164eb9cbd083e8a2503c4e");

        // RFC 8439, Section 2.3.2: complete ChaCha20 block-function known answer.
        REQUIRE_EQUAL(el::cryptology::impl::chacha20::block(key.span(), nonce.span(), 1U), expected);

        auto backend = PortableChaCha20Backend{key.span(), nonce.span()};
        const auto batch = backend.generateBlocks(1U, 1U);
        REQUIRE_EQUAL(
            el::mem::ByteBlock::fromSpan(batch.span(el::unit::ByteIndex{}, el::unit::ByteLength{64U})),
            el::mem::ByteBlock{expected});
    }

    void testPoly1305Rfc8439Section252() {
        const auto key = arrayFromHex<32>("85d6be7857556d337f4452fe42d506a80103808afb0db2fd4abff6af4149f51b");
        const auto message = bytesFromHex("43727970746f6772617068696320466f72756d2052657365617263682047726f7570");
        const auto expected = arrayFromHex<16>("a8061dc1305136c6c22b8baf0c0127a9");
        auto poly1305 = PortablePoly1305{key.span()};

        // RFC 8439, Section 2.5.2: exercise full and partial block accumulation in separate updates.
        poly1305.update(message.span(el::unit::ByteIndex{}, el::unit::ByteLength{16U}));
        poly1305.update(message.span(el::unit::ByteIndex{16U}, el::unit::ByteLength{1U}));
        poly1305.update(message.span(el::unit::ByteIndex{17U}, el::unit::ByteLength{17U}));
        REQUIRE_EQUAL(poly1305.finalize(), expected);
    }

    void testPoly1305OneTimeKeyRfc8439Section262() {
        const auto key = arrayFromHex<32>("808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f");
        const auto nonce = arrayFromHex<12>("000000000001020304050607");
        const auto expected = arrayFromHex<32>("8ad5a08b905f81cc815040274ab29471a833b637e3fd0da508dbb8e2fdd1a646");
        auto backend = PortableChaCha20Backend{key.span(), nonce.span()};

        // RFC 8439, Section 2.6.2: counter zero supplies the first 256 bits of the one-time Poly1305 key.
        auto firstBlock = backend.generateBlocks(0U, 1U);
        REQUIRE_EQUAL(
            el::mem::ByteArray<32>::fromSpanOrThrow(firstBlock.span(el::unit::ByteIndex{}, el::unit::ByteLength{32U})),
            expected);
        firstBlock.secureErase();
    }
};
