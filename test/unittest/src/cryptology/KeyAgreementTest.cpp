// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/CryptologyError.hpp>
#include <erbsland/cryptology/Hkdf.hpp>
#include <erbsland/cryptology/impl/algorithm/x25519/X25519.hpp>
#include <erbsland/cryptology/keys/KeyAgreementAlgorithm.hpp>
#include <erbsland/cryptology/keys/KeyAgreementPrivateKey.hpp>
#include <erbsland/cryptology/keys/KeyAgreementPublicKey.hpp>
#include <erbsland/cryptology/keys/KeyAgreementSharedSecret.hpp>
#include <erbsland/cryptology/protected_data/ProtectedDataMode.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/mem/impl/SecureErase.hpp>

#include <algorithm>
#include <atomic>
#include <type_traits>

using namespace el::cryptology;

TESTED_TARGETS(KeyAgreementAlgorithm KeyAgreementPublicKey KeyAgreementPrivateKey KeyAgreementSharedSecret X25519)
class KeyAgreementTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    static_assert(!std::is_copy_constructible_v<KeyAgreementPrivateKey>);
    static_assert(!std::is_copy_assignable_v<KeyAgreementPrivateKey>);
    static_assert(!std::is_copy_constructible_v<KeyAgreementSharedSecret>);
    static_assert(!std::is_copy_assignable_v<KeyAgreementSharedSecret>);

    class EraseObserverGuard final {
    public:
        EraseObserverGuard() {
            _scalarEraseCount = 0U;
            _limbEraseCount = 0U;
            _nonzeroEraseCount = 0U;
            el::mem::impl::setSecureEraseObserver(observeErase);
        }
        ~EraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }
    };

    static void observeErase(const std::span<const std::byte> bytes) noexcept {
        const auto isZero =
            std::ranges::all_of(bytes, [](const std::byte value) noexcept { return value == std::byte{}; });
        if (!isZero) {
            ++_nonzeroEraseCount;
        }
        if (bytes.size() == 32U) {
            ++_scalarEraseCount;
        }
        if (bytes.size() == 10U * sizeof(uint64_t)) {
            ++_limbEraseCount;
        }
    }

    inline static std::atomic<std::size_t> _scalarEraseCount{0U};
    inline static std::atomic<std::size_t> _limbEraseCount{0U};
    inline static std::atomic<std::size_t> _nonzeroEraseCount{0U};

public:
    void testAlgorithmMetadataAndParsing() {
        using namespace el::text::literals;
        const auto algorithm = KeyAgreementAlgorithm{KeyAgreementAlgorithm::X25519};
        REQUIRE_EQUAL(algorithm.publicKeySize(), el::ByteLength{32U});
        REQUIRE_EQUAL(algorithm.privateKeySize(), el::ByteLength{32U});
        REQUIRE_EQUAL(algorithm.sharedSecretSize(), el::ByteLength{32U});
        REQUIRE_EQUAL(algorithm.toString(), "x25519"_el);
        REQUIRE_EQUAL(KeyAgreementAlgorithm::fromStringOrThrow("x25519"_el), algorithm);
        REQUIRE_EQUAL(KeyAgreementAlgorithm::all().size(), std::size_t{1U});
    }

    void testRfc7748AliceAndBob() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto alicePrivateData = bytesFromHex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a");
        const auto alicePublicData = bytesFromHex("8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a");
        const auto bobPrivateData = bytesFromHex("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb");
        const auto bobPublicData = bytesFromHex("de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f");
        const auto expectedShared = bytesFromHex("4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742");
        const auto algorithm = KeyAgreementAlgorithm{KeyAgreementAlgorithm::X25519};
        const auto alicePrivate = KeyAgreementPrivateKey::fromBytes(algorithm, alicePrivateData.span());
        const auto bobPrivate = KeyAgreementPrivateKey::fromBytes(algorithm, bobPrivateData.span());
        REQUIRE_EQUAL(alicePrivate.publicKey().data(), alicePublicData);
        REQUIRE_EQUAL(bobPrivate.publicKey().data(), bobPublicData);

        const auto aliceShared = alicePrivate.agree(bobPrivate.publicKey());
        const auto bobShared = bobPrivate.agree(alicePrivate.publicKey());
        const auto hkdf = Hkdf{HashAlgorithm::Sha2_256};
        REQUIRE_EQUAL(hkdf.extract(aliceShared), hkdf.extract(expectedShared.span()));
        REQUIRE_EQUAL(hkdf.extract(bobShared), hkdf.extract(expectedShared.span()));
    }

    void testAllZeroAndInvalidStates() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto algorithm = KeyAgreementAlgorithm{KeyAgreementAlgorithm::X25519};
        const auto privateData = bytesFromHex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a");
        const auto privateKey = KeyAgreementPrivateKey::fromBytes(algorithm, privateData.span());
        const auto zeroPeer = KeyAgreementPublicKey{algorithm, el::ByteBlock{el::ByteLength{32U}}.span()};
        REQUIRE_THROWS_AS(CryptologyError, privateKey.agree(zeroPeer));
        REQUIRE_THROWS_AS(el::err::LogicError, KeyAgreementPrivateKey{}.publicKey());
        REQUIRE_THROWS_AS(el::err::LogicError, KeyAgreementPrivateKey{}.agree(zeroPeer));
        REQUIRE_THROWS_AS(
            el::err::ParameterError, KeyAgreementPublicKey{algorithm, el::ByteBlock{el::ByteLength{31U}}.span()});
        REQUIRE_THROWS_AS(el::err::LogicError, Hkdf{HashAlgorithm::Sha2_256}.extract(KeyAgreementSharedSecret{}));
    }

    void testScalarClampingAndInputMasking() {
        const auto applicationScope = ApplicationTestScope<>{};
        el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
        const auto algorithm = KeyAgreementAlgorithm{KeyAgreementAlgorithm::X25519};
        const auto privateData = bytesFromHex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a");
        auto equivalentPrivateData = el::ByteBlockEditor{privateData};
        equivalentPrivateData.set(
            el::ByteIndex::zero(), privateData.getOrThrow(el::ByteIndex::zero()) ^ el::Byte{0x07U});
        equivalentPrivateData.set(el::ByteIndex{31U}, privateData.getOrThrow(el::ByteIndex{31U}) ^ el::Byte{0xc0U});
        const auto firstPrivate = KeyAgreementPrivateKey::fromBytes(algorithm, privateData.span());
        const auto secondPrivate = KeyAgreementPrivateKey::fromBytes(algorithm, equivalentPrivateData.span());
        REQUIRE_EQUAL(firstPrivate.publicKey().data(), secondPrivate.publicKey().data());

        const auto peerData = bytesFromHex("de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f");
        auto maskedPeerData = el::ByteBlockEditor{peerData};
        maskedPeerData.set(el::ByteIndex{31U}, peerData.getOrThrow(el::ByteIndex{31U}) | el::Byte{0x80U});
        const auto firstSecret = firstPrivate.agree(KeyAgreementPublicKey{algorithm, peerData.span()});
        const auto secondSecret = firstPrivate.agree(KeyAgreementPublicKey{algorithm, maskedPeerData.span()});
        const auto hkdf = Hkdf{HashAlgorithm::Sha2_256};
        REQUIRE_EQUAL(hkdf.extract(firstSecret), hkdf.extract(secondSecret));
    }

    void testX25519ScratchErasure() {
        const auto privateData = bytesFromHex("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a");
        const auto peerData = bytesFromHex("de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f");
        const auto observer = EraseObserverGuard{};
        static_cast<void>(el::cryptology::impl::x25519::agree(privateData.span(), peerData.span()));
        REQUIRE(_scalarEraseCount.load() >= 1U);
        REQUIRE(_limbEraseCount.load() >= 1U);
        REQUIRE_EQUAL(_nonzeroEraseCount.load(), std::size_t{0U});
    }
};
