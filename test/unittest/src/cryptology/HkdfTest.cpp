// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/Hkdf.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/unit/ByteLength.hpp>

#include <type_traits>

using namespace el::cryptology;

TESTED_TARGETS(Hkdf)
class HkdfTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    static_assert(std::is_copy_constructible_v<Hkdf>);
    static_assert(std::is_copy_assignable_v<Hkdf>);

public:
    void testSupportedAlgorithms() {
        for (const auto algorithm : HashAlgorithm::all()) {
            if (algorithm == HashAlgorithm::Sha2_256 || algorithm == HashAlgorithm::Sha2_384) {
                const auto hkdf = Hkdf{algorithm};
                REQUIRE_EQUAL(hkdf.algorithm(), algorithm);
            } else {
                REQUIRE_THROWS_AS(el::err::ParameterError, (Hkdf{algorithm}));
            }
        }
    }

    void testRfc5869Sha256CaseOne() {
        const auto inputKeyMaterial =
            el::mem::ByteBlock{el::mem::ByteBlockEditor{el::unit::ByteLength{22U}, el::mem::Byte{0x0bU}}};
        const auto salt = bytesFromHex("000102030405060708090a0b0c");
        const auto info = bytesFromHex("f0f1f2f3f4f5f6f7f8f9");
        const auto expectedPrk = bytesFromHex("077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5");
        const auto expectedOkm =
            bytesFromHex("3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865");

        const auto hkdf = Hkdf{HashAlgorithm::Sha2_256};
        const auto prk = hkdf.extract(inputKeyMaterial.span(), salt.span());
        REQUIRE(prk.isSensitive());
        REQUIRE_EQUAL(prk, expectedPrk);
        const auto okm = hkdf.expand(prk.span(), info.span(), el::unit::ByteLength{42U});
        REQUIRE(okm.isSensitive());
        REQUIRE_EQUAL(okm, expectedOkm);
    }

    void testRfc5869EmptySaltAndInfo() {
        const auto inputKeyMaterial =
            el::mem::ByteBlock{el::mem::ByteBlockEditor{el::unit::ByteLength{22U}, el::mem::Byte{0x0bU}}};
        const auto expectedPrk = bytesFromHex("19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04");
        const auto expectedOkm =
            bytesFromHex("8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8");

        const auto hkdf = Hkdf{HashAlgorithm::Sha2_256};
        const auto prk = hkdf.extract(inputKeyMaterial.span());
        REQUIRE_EQUAL(prk, expectedPrk);
        REQUIRE_EQUAL(hkdf.expand(prk.span(), {}, el::unit::ByteLength{42U}), expectedOkm);

        const auto zeroSalt = el::mem::ByteBlock{HashAlgorithm{HashAlgorithm::Sha2_256}.digestSize()};
        REQUIRE_EQUAL(prk, hkdf.extract(inputKeyMaterial.span(), zeroSalt.span()));
    }

    void testSha384IndependentVector() {
        // RFC 5869 case-one inputs, independently cross-checked with Python hashlib and PHP hash_hkdf.
        const auto inputKeyMaterial =
            el::mem::ByteBlock{el::mem::ByteBlockEditor{el::unit::ByteLength{22U}, el::mem::Byte{0x0bU}}};
        const auto salt = bytesFromHex("000102030405060708090a0b0c");
        const auto info = bytesFromHex("f0f1f2f3f4f5f6f7f8f9");
        const auto expectedPrk = bytesFromHex(
            "704b39990779ce1dc548052c7dc39f303570dd13fb39f7acc564680bef80e8dec70ee9a7e1f3e293ef68eceb072a5ade");
        const auto expectedOkm =
            bytesFromHex("9b5097a86038b805309076a44b3a9f38063e25b516dcbf369f394cfab43685f748b6457763e4f0204fc5");

        const auto hkdf = Hkdf{HashAlgorithm::Sha2_384};
        const auto prk = hkdf.extract(inputKeyMaterial.span(), salt.span());
        REQUIRE_EQUAL(prk, expectedPrk);
        REQUIRE_EQUAL(hkdf.expand(prk.span(), info.span(), el::unit::ByteLength{42U}), expectedOkm);
    }

    void testBoundaries() {
        for (const auto algorithm : {HashAlgorithm{HashAlgorithm::Sha2_256}, HashAlgorithm{HashAlgorithm::Sha2_384}}) {
            const auto hkdf = Hkdf{algorithm};
            const auto pseudoRandomKey = el::mem::ByteBlock{algorithm.digestSize()};
            const auto empty = hkdf.expand(pseudoRandomKey.span(), {}, el::unit::ByteLength::zero());
            REQUIRE(empty.isEmpty());

            const auto maximum = el::unit::ByteLength::fromSizeT(algorithm.digestSize().toSizeT() * 255U);
            const auto largest = hkdf.expand(pseudoRandomKey.span(), {}, maximum);
            REQUIRE_EQUAL(largest.length(), maximum);
            REQUIRE(largest.isSensitive());
            REQUIRE_THROWS_AS(
                el::err::ParameterError,
                hkdf.expand(pseudoRandomKey.span(), {}, maximum + el::unit::ByteLength::one()));

            const auto shortKey = el::mem::ByteBlock{algorithm.digestSize() - el::unit::ByteLength::one()};
            REQUIRE_THROWS_AS(el::err::ParameterError, hkdf.expand(shortKey.span(), {}, el::unit::ByteLength::one()));
        }
    }
};
