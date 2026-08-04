// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/CryptologyError.hpp>
#include <erbsland/cryptology/impl/symmetric/ChaCha20Poly1305State.hpp>
#include <erbsland/cryptology/symmetric/SymmetricDecryptor.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptor.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/mem/impl/SecureErase.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <limits>
#include <span>

namespace erbsland::cryptology::impl {

/// Unit-test access for otherwise unreachable RFC 8439 length and counter boundaries.
class ChaCha20Poly1305StateTestAccess final {
public:
    static void setPayloadPosition(
        ChaCha20Poly1305State &state, const uint64_t length, const uint64_t nextCounter) noexcept {
        state._payloadLength = length;
        state._nextCounter = nextCounter;
        state._keyStreamOffset = 0U;
        state._keyStreamLength = 0U;
    }

    static void setAuthenticatedDataLength(ChaCha20Poly1305State &state, const uint64_t length) noexcept {
        state._authenticatedDataLength = length;
    }

    [[nodiscard]] static constexpr auto maximumPayloadLength() noexcept -> uint64_t {
        return ChaCha20Poly1305State::maximumPayloadLength;
    }
};

}

using namespace el::cryptology;

TESTED_TARGETS(ChaCha20Poly1305State ChaCha20Poly1305EncryptorData ChaCha20Poly1305DecryptorData)
class ChaCha20Poly1305Test final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    class EraseObserverGuard final {
    public:
        EraseObserverGuard() {
            _eraseCount.store(0U);
            _allErased.store(true);
            el::mem::impl::setSecureEraseObserver(observeErase);
        }
        ~EraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }
    };

    static void observeErase(const std::span<const std::byte> bytes) noexcept {
        _eraseCount.fetch_add(1U);
        if (!std::ranges::all_of(bytes, [](const std::byte value) noexcept { return value == std::byte{}; })) {
            _allErased.store(false);
        }
    }

    [[nodiscard]] static auto key() -> SymmetricKey {
        return SymmetricKey{bytesFromHex("808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f")};
    }

    [[nodiscard]] static auto nonce() -> SymmetricNonce {
        return SymmetricNonce{bytesFromHex("070000004041424344454647")};
    }

    [[nodiscard]] static auto aad() -> el::mem::ByteBlock { return bytesFromHex("50515253c0c1c2c3c4c5c6c7"); }

    [[nodiscard]] static auto plaintext() -> el::mem::ByteBlock {
        return bytesFromHex(
            "4c616469657320616e642047656e746c656d656e206f662074686520636c6173"
            "73206f66202739393a204966204920636f756c64206f6666657220796f75206f"
            "6e6c79206f6e652074697020666f7220746865206675747572652c2073756e73"
            "637265656e20776f756c642062652069742e");
    }

    [[nodiscard]] static auto ciphertext() -> el::mem::ByteBlock {
        return bytesFromHex(
            "d31a8d34648e60db7b86afbc53ef7ec2a4aded51296e08fea9e2b5a736ee62d6"
            "3dbea45e8ca9671282fafb69da92728b1a71de0a9e060b2905d6a5b67ecd3b36"
            "92ddbd7f2d778b8c9803aee328091b58fab324e4fad675945585808b4831d7bc"
            "3ff4def08e4b7a9de576d26586cec64b6116");
    }

public:
    void testStreamingCipherRfc8439Section242() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto streamKey =
            SymmetricKey{bytesFromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")};
        const auto streamNonce = SymmetricNonce{bytesFromHex("000000000000004a00000000")};
        const auto expected = bytesFromHex(
            "6e2e359a2568f98041ba0728dd0d6981e97e7aec1d4360c20a27afccfd9fae0b"
            "f91b65c5524733ab8f593dabcd62b3571639d624e65152ab8f530c359f0861d8"
            "07ca0dbf500d6a6156a38e088a22b65e52bc514d16ccf806818ce91ab7793736"
            "5af90bbf74a35be6b40b8eedf2785e42874d");
        const auto message = plaintext();
        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::ChaCha20Poly1305, streamKey, streamNonce};
        auto actual = el::mem::ByteBlockEditor{};

        // RFC 8439, Section 2.4.2: update boundaries do not alter the counter-one ChaCha20 stream.
        actual.append(encryptor.encrypt(message.span(el::unit::ByteIndex{}, el::unit::ByteLength{1U})));
        actual.append(encryptor.encrypt(message.span(el::unit::ByteIndex{1U}, el::unit::ByteLength{63U})));
        actual.append(encryptor.encrypt(message.span().subspan(64U)));
        REQUIRE_EQUAL(actual, expected);
        REQUIRE(encryptor.finalize().isEmpty());
    }

    void testCompleteRfc8439Section282Vector() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::ChaCha20Poly1305, key(), nonce()};
        encryptor.addAuthenticatedData(aad());
        REQUIRE_EQUAL(encryptor.encrypt(plaintext()), ciphertext());
        REQUIRE(encryptor.finalize().isEmpty());
        REQUIRE_EQUAL(encryptor.tag().data(), bytesFromHex("1ae10b594f09e26a7e902ecbd0600691"));

        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::ChaCha20Poly1305, key(), nonce()};
        decryptor.addAuthenticatedData(aad());
        REQUIRE_EQUAL(decryptor.decrypt(ciphertext()), plaintext());
        REQUIRE(decryptor.finalize(encryptor.tag()).isEmpty());
    }

    void testIrregularStreamingAndEmptyPayloadKeepsAadOpen() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::ChaCha20Poly1305, key(), nonce()};
        encryptor.addAuthenticatedData(aad().span(el::unit::ByteIndex{}, el::unit::ByteLength{3U}));
        REQUIRE(encryptor.encrypt(el::mem::ConstByteSpan{}).isEmpty());
        encryptor.addAuthenticatedData(aad().span(el::unit::ByteIndex{3U}, el::unit::ByteLength{9U}));

        auto actualCiphertext = el::mem::ByteBlockEditor{};
        for (auto index = std::size_t{}; index < plaintext().length().toSizeT(); ++index) {
            actualCiphertext.append(
                encryptor.encrypt(plaintext().span(el::unit::ByteIndex{index}, el::unit::ByteLength::one())));
        }
        static_cast<void>(encryptor.finalize());
        REQUIRE_EQUAL(actualCiphertext, ciphertext());
        REQUIRE_EQUAL(encryptor.tag().data(), bytesFromHex("1ae10b594f09e26a7e902ecbd0600691"));
    }

    void testAuthenticationFailureIsGenericAndFacadeFails() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto invalidTag = el::mem::ByteBlockEditor{bytesFromHex("1ae10b594f09e26a7e902ecbd0600691")};
        invalidTag.xorAt(el::unit::ByteIndex{8U}, el::mem::Byte{1U});
        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::ChaCha20Poly1305, key(), nonce()};
        decryptor.addAuthenticatedData(aad());
        static_cast<void>(decryptor.decrypt(ciphertext()));
        REQUIRE_THROWS_AS(CryptologyError, decryptor.finalize(SymmetricTag{invalidTag}));
        REQUIRE_THROWS_AS(el::err::LogicError, decryptor.decrypt(el::mem::ConstByteSpan{}));
    }

    void testAadCiphertextAndEveryTagRegionAreAuthenticated() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto expectedTag = SymmetricTag{bytesFromHex("1ae10b594f09e26a7e902ecbd0600691")};
        const auto requireFailure = [&](const el::mem::ByteBlock &testAad,
                                        const el::mem::ByteBlock &testCiphertext,
                                        const SymmetricTag &testTag) -> void {
            auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::ChaCha20Poly1305, key(), nonce()};
            decryptor.addAuthenticatedData(testAad);
            static_cast<void>(decryptor.decrypt(testCiphertext));
            REQUIRE_THROWS_AS(CryptologyError, decryptor.finalize(testTag));
        };

        auto modifiedAad = el::mem::ByteBlockEditor{aad()};
        modifiedAad.xorAt(el::unit::ByteIndex{3U}, el::mem::Byte{1U});
        requireFailure(modifiedAad, ciphertext(), expectedTag);

        auto modifiedCiphertext = el::mem::ByteBlockEditor{ciphertext()};
        modifiedCiphertext.xorAt(el::unit::ByteIndex{64U}, el::mem::Byte{1U});
        requireFailure(aad(), modifiedCiphertext, expectedTag);

        for (const auto index : {0U, 4U, 8U, 12U, 15U}) {
            auto modifiedTag = el::mem::ByteBlockEditor{expectedTag.data()};
            modifiedTag.xorAt(el::unit::ByteIndex{index}, el::mem::Byte{1U});
            requireFailure(aad(), ciphertext(), SymmetricTag{modifiedTag});
        }
    }

    void testEmptyAndAadOnlyMessages() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto emptyKey =
            SymmetricKey{bytesFromHex("0000000000000000000000000000000000000000000000000000000000000000")};
        const auto emptyNonce = SymmetricNonce{bytesFromHex("000000000000000000000000")};

        auto empty = SymmetricEncryptor{SymmetricEncryptionType::ChaCha20Poly1305, emptyKey, emptyNonce};
        static_cast<void>(empty.finalize());
        REQUIRE_EQUAL(empty.tag().data(), bytesFromHex("4eb972c9a8fb3a1b382bb4d36f5ffad1"));

        auto aadOnly = SymmetricEncryptor{SymmetricEncryptionType::ChaCha20Poly1305, emptyKey, emptyNonce};
        aadOnly.addAuthenticatedData(bytesFromHex("00"));
        static_cast<void>(aadOnly.finalize());
        REQUIRE_EQUAL(aadOnly.tag().data().length(), el::unit::ByteLength{16U});
        REQUIRE_NOT_EQUAL(aadOnly.tag().data(), empty.tag().data());

        auto aadOnlyDecryptor = SymmetricDecryptor{SymmetricEncryptionType::ChaCha20Poly1305, emptyKey, emptyNonce};
        aadOnlyDecryptor.addAuthenticatedData(bytesFromHex("00"));
        REQUIRE(aadOnlyDecryptor.finalize(aadOnly.tag()).isEmpty());
    }

    void testCounterExhaustionAndLengthChecksPrecedeOutput() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto stateKey = bytesFromHex("808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f");
        const auto stateNonce = bytesFromHex("070000004041424344454647");
        auto state = el::cryptology::impl::ChaCha20Poly1305State{stateKey.span(), stateNonce.span()};
        const auto maximum = el::cryptology::impl::ChaCha20Poly1305StateTestAccess::maximumPayloadLength();
        el::cryptology::impl::ChaCha20Poly1305StateTestAccess::setPayloadPosition(state, maximum - 64U, 0xffffffffU);
        REQUIRE_EQUAL(
            state.transform(el::mem::ByteBlock{el::unit::ByteLength{64U}}.span(), true).length(),
            el::unit::ByteLength{64U});
        REQUIRE_THROWS_AS(CryptologyError, state.transform(bytesFromHex("00").span(), true));

        el::cryptology::impl::ChaCha20Poly1305StateTestAccess::setAuthenticatedDataLength(
            state, std::numeric_limits<uint64_t>::max());
        REQUIRE_THROWS_AS(CryptologyError, state.addAuthenticatedData(bytesFromHex("00").span()));
    }

    void testSecureEraseCoversRetainedState() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto stateKey = bytesFromHex("808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f");
        const auto stateNonce = bytesFromHex("070000004041424344454647");
        auto state = el::cryptology::impl::ChaCha20Poly1305State{stateKey.span(), stateNonce.span()};
        static_cast<void>(state.transform(plaintext().span(), true));
        {
            const auto observer = EraseObserverGuard{};
            state.secureErase();
            REQUIRE_GREATER(_eraseCount.load(), 0U);
            REQUIRE(_allErased.load());
        }
    }

private:
    inline static std::atomic<std::size_t> _eraseCount{};
    inline static std::atomic<bool> _allErased{true};
};
