// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/Hmac.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/mem/impl/SecureErase.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

using namespace el::cryptology;

TESTED_TARGETS(Hmac)
class HmacTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    class EraseObserverGuard final {
    public:
        EraseObserverGuard() {
            eraseCount = 0U;
            allErased = true;
            el::mem::impl::setSecureEraseObserver(observeErase);
        }
        ~EraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }

        // deletions
        EraseObserverGuard(const EraseObserverGuard &) = delete;
        EraseObserverGuard(EraseObserverGuard &&) = delete;
        auto operator=(const EraseObserverGuard &) -> EraseObserverGuard & = delete;
        auto operator=(EraseObserverGuard &&) -> EraseObserverGuard & = delete;

    public:
        static inline std::size_t eraseCount{0U};
        static inline bool allErased{true};

    private:
        static void observeErase(const std::span<const std::byte> memory) noexcept {
            ++eraseCount;
            for (const auto value : memory) {
                if (value != std::byte{}) {
                    allErased = false;
                }
            }
        }
    };

    struct RfcVector {
        HashAlgorithm algorithm;
        std::string key;
        std::string message;
        std::string expected;
    };

    struct BoundaryVector {
        HashAlgorithm algorithm;
        std::size_t blockSize;
        std::string expected;
    };

    static_assert(!std::is_copy_constructible_v<Hmac>);
    static_assert(!std::is_copy_assignable_v<Hmac>);
    static_assert(std::is_nothrow_move_constructible_v<Hmac>);
    static_assert(std::is_nothrow_move_assignable_v<Hmac>);

public:
    void testInvalidPlaceholder() {
        auto hmac = Hmac{};
        REQUIRE_FALSE(hmac.isValid());
        hmac.secureErase();
        REQUIRE_THROWS_AS(el::err::LogicError, hmac.algorithm());
        REQUIRE_THROWS_AS(el::err::LogicError, hmac.reset());
        REQUIRE_THROWS_AS(el::err::LogicError, hmac.update(el::mem::ConstByteSpan{}));
        REQUIRE_THROWS_AS(el::err::LogicError, hmac.finalize());
        REQUIRE_THROWS_AS(el::err::LogicError, hmac.verify(el::mem::ConstByteSpan{}));
    }

    void testSupportedAlgorithms() {
        const auto empty = el::mem::ByteBlock{};
        for (const auto algorithm : HashAlgorithm::all()) {
            if (algorithm == HashAlgorithm::Sha2_256 || algorithm == HashAlgorithm::Sha2_384) {
                auto hmac = Hmac{algorithm, empty.span()};
                REQUIRE(hmac.isValid());
                REQUIRE_EQUAL(hmac.algorithm(), algorithm);
                REQUIRE_EQUAL(hmac.finalize().length(), algorithm.digestSize());
            } else {
                REQUIRE_THROWS_AS(el::err::ParameterError, (Hmac{algorithm, empty.span()}));
            }
        }
    }

    void testRfc4231Vectors() {
        const auto vectors = std::array<RfcVector, 6>{
            RfcVector{
                HashAlgorithm::Sha2_256,
                "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
                "4869205468657265",
                "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"},
            RfcVector{
                HashAlgorithm::Sha2_384,
                "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
                "4869205468657265",
                "afd03944d84895626b0825f4ab46907f15f9dadbe4101ec682aa034c7cebc59c"
                "faea9ea9076ede7f4af152e8b2fa9cb6"},
            RfcVector{
                HashAlgorithm::Sha2_256,
                "4a656665",
                "7768617420646f2079612077616e7420666f72206e6f7468696e673f",
                "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"},
            RfcVector{
                HashAlgorithm::Sha2_384,
                "4a656665",
                "7768617420646f2079612077616e7420666f72206e6f7468696e673f",
                "af45d2e376484031617f78d2b58a6b1b9c7ef464f5a01b47e42ec3736322445e"
                "8e2240ca5e69e2c78b3239ecfab21649"},
            RfcVector{
                HashAlgorithm::Sha2_256,
                std::string(262U, 'a'),
                "54657374205573696e67204c6172676572205468616e20426c6f636b2d53697a65204b6579202d2048617368204b65"
                "79204669727374",
                "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54"},
            RfcVector{
                HashAlgorithm::Sha2_384,
                std::string(262U, 'a'),
                "54657374205573696e67204c6172676572205468616e20426c6f636b2d53697a65204b6579202d2048617368204b65"
                "79204669727374",
                "4ece084485813e9088d2c63a041bc5b44f9ef1012a2b588f3cd11f05033ac4c60c2ef6ab4030fe8296248df163f44952"}};

        for (const auto &[algorithm, keyText, messageText, expectedText] : vectors) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    const auto key = bytesFromHex(keyText);
                    const auto message = bytesFromHex(messageText);
                    const auto expected = bytesFromHex(expectedText);
                    auto hmac = Hmac{algorithm, key.span()};
                    hmac.update(message);
                    REQUIRE_EQUAL(hmac.finalize(), expected);
                },
                [&]() -> std::string {
                    return (algorithm == HashAlgorithm::Sha2_256 ? "SHA-256 key-hex=" : "SHA-384 key-hex=") +
                        std::to_string(keyText.size());
                });
        }
    }

    void testBlockSizedKeys() {
        const auto vectors = std::array<BoundaryVector, 2>{
            BoundaryVector{
                HashAlgorithm::Sha2_256, 64U, "04660fc313657aa3500078e1f2788cc4e328654092b137f946516e4d7a17adae"},
            BoundaryVector{
                HashAlgorithm::Sha2_384,
                128U,
                "9056cf7bd13ae53f2821ff3f5d2c56b4d062c6c8a67eb9a27ad018cdf4316d74"
                "4d0676e31381b31069af89c49ec5c1bb"}};

        for (const auto &[algorithm, blockSize, expectedText] : vectors) {
            auto key = el::mem::ByteBlockEditor{el::unit::ByteLength::fromSizeT(blockSize)};
            for (auto index = std::size_t{}; index < blockSize; ++index) {
                key.set(el::unit::ByteIndex::fromSizeT(index), el::mem::Byte{static_cast<uint8_t>(index)});
            }
            auto hmac = Hmac{algorithm, el::mem::ByteBlock{key}};
            hmac.update(el::text::String{"boundary"});
            REQUIRE_EQUAL(hmac.finalize(), bytesFromHex(expectedText));
        }
    }

    void testSegmentedAndTextInput() {
        const auto key = bytesFromHex("000102030405060708090a0b0c0d0e0f");
        auto whole = Hmac{HashAlgorithm::Sha2_256, key.span()};
        whole.update(el::text::String{"Grüezi 😀"});

        const auto bytes = el::unittest::th::stdStringFromHex("47 72 C3 BC 65 7A 69 20 F0 9F 98 80");
        auto segmented = Hmac{HashAlgorithm::Sha2_256, key.span()};
        const auto span = el::mem::toConstByteSpan(std::span<const char>{bytes});
        segmented.update(span.first(3U));
        segmented.update(el::mem::ByteBlock::fromSpan(span.subspan(3U, 4U)));
        segmented.update(span.subspan(7U));
        REQUIRE_EQUAL(segmented.finalize(), whole.finalize());
    }

    void testLifecycleAndVerification() {
        const auto key = bytesFromHex("4a656665");
        const auto message = bytesFromHex("7768617420646f2079612077616e7420666f72206e6f7468696e673f");
        const auto expected = bytesFromHex("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
        auto hmac = Hmac{HashAlgorithm::Sha2_256, key.span()};
        hmac.update(message);
        const auto result = hmac.finalize();
        REQUIRE_FALSE(result.isSensitive());
        REQUIRE_EQUAL(hmac.finalize(), result);
        REQUIRE(hmac.verify(expected));
        REQUIRE_FALSE(hmac.verify(expected.span().first(16U)));
        REQUIRE_FALSE(hmac.verify(bytesFromHex("00bdc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843")));
        REQUIRE_THROWS_AS(el::err::LogicError, hmac.update(message));

        hmac.reset();
        hmac.update(message);
        REQUIRE(hmac.verify(expected));

        hmac.reset();
        hmac.update(message);
        REQUIRE_FALSE(hmac.verify(expected.span().first(16U)));
        hmac.update(el::mem::ConstByteSpan{});
        REQUIRE(hmac.verify(expected));
    }

    void testMoveAndSensitiveOwningKey() {
        auto key = bytesFromHex("000102030405060708090a0b0c0d0e0f");
        REQUIRE_FALSE(key.isSensitive());
        auto source = Hmac{HashAlgorithm::Sha2_256, key};
        REQUIRE(key.isSensitive());
        source.update(el::mem::ConstByteSpan{});
        auto destination = std::move(source);
        REQUIRE_FALSE(source.isValid());
        REQUIRE(destination.isValid());
        REQUIRE_EQUAL(destination.algorithm(), HashAlgorithm::Sha2_256);

        auto replacement = Hmac{HashAlgorithm::Sha2_384, key.span()};
        replacement = std::move(destination);
        REQUIRE_FALSE(destination.isValid());
        REQUIRE_EQUAL(replacement.algorithm(), HashAlgorithm::Sha2_256);
    }

    void testSecureErase() {
        const auto key = bytesFromHex("000102030405060708090a0b0c0d0e0f");
        auto hmac = Hmac{HashAlgorithm::Sha2_384, key.span()};
        hmac.update(bytesFromHex("010203040506070809"));
        {
            const auto observer = EraseObserverGuard{};
            hmac.secureErase();
            REQUIRE(EraseObserverGuard::eraseCount > 0U);
            REQUIRE(EraseObserverGuard::allErased);
        }
        REQUIRE_FALSE(hmac.isValid());
        REQUIRE_THROWS_AS(el::err::LogicError, hmac.finalize());
    }
};
