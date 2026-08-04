// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/cryptology/impl/tls/TlsRecordState.hpp>
#include <erbsland/cryptology/impl/tls/TlsRecordTestAccess.hpp>
#include <erbsland/cryptology/tls/TlsCipherSuite.hpp>
#include <erbsland/cryptology/tls_record/TlsRecordDecryptor.hpp>
#include <erbsland/cryptology/tls_record/TlsRecordEncryptor.hpp>
#include <erbsland/cryptology/tls_record/TlsRecordError.hpp>
#include <erbsland/cryptology/tls_record/TlsTrafficSecret.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/text/Literals.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

using namespace el::cryptology;
using namespace el::text::literals;
using el::mem::ByteBlock;
using el::unit::ByteLength;

TESTED_TARGETS(
    TlsCipherSuite TlsTrafficSecret TlsRecordContentType TlsRecordErrorCategory TlsRecordError TlsRecordPlaintext
        TlsRecordEncryptor TlsRecordDecryptor TlsRecordState TlsRecordTestAccess)
class TlsRecordProtectionTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    [[nodiscard]] static auto makeSecret(const TlsCipherSuite suite, ByteBlock bytes) -> TlsTrafficSecret {
        return TlsTrafficSecret::fromBytes(suite.hashAlgorithm(), std::move(bytes));
    }

    [[nodiscard]] static auto sequentialBytes(const std::size_t count, const bool descending = false) -> ByteBlock {
        auto result = el::mem::ByteBlockEditor{};
        for (auto index = std::size_t{0U}; index < count; ++index) {
            const auto value = descending ? 0xffU - index : index;
            result.append(el::Byte{static_cast<uint8_t>(value)});
        }
        return result;
    }

    template <typename Function>
    void requireRecordError(const TlsRecordErrorCategory expected, Function function) {
        auto wasThrown = false;
        try {
            function();
        } catch (const TlsRecordError &error) {
            wasThrown = true;
            REQUIRE_EQUAL(error.category(), expected);
        }
        REQUIRE(wasThrown);
    }

    void requireRoundTrip(
        const TlsCipherSuite suite,
        const TlsRecordContentType type,
        const ByteBlock &content,
        const ByteLength padding = ByteLength::zero()) {
        const auto secretBytes = ByteBlock{suite.hashAlgorithm().digestSize(), 0x5aU};
        auto encryptor = TlsRecordEncryptor{suite, makeSecret(suite, secretBytes)};
        auto decryptor = TlsRecordDecryptor{suite, makeSecret(suite, secretBytes)};
        const auto record = encryptor.protect(type, content.span(), padding);
        auto plaintext = decryptor.unprotect(record.span());
        REQUIRE_EQUAL(plaintext.type(), type);
        REQUIRE_EQUAL(plaintext.content(), content);
        REQUIRE(plaintext.content().isSensitive() || plaintext.content().isEmpty());
    }

    static_assert(!std::is_copy_constructible_v<TlsTrafficSecret>);
    static_assert(std::is_nothrow_move_constructible_v<TlsTrafficSecret>);
    static_assert(!std::is_copy_constructible_v<TlsRecordPlaintext>);
    static_assert(std::is_nothrow_move_constructible_v<TlsRecordPlaintext>);
    static_assert(!std::is_copy_constructible_v<TlsRecordEncryptor>);
    static_assert(std::is_nothrow_move_constructible_v<TlsRecordEncryptor>);
    static_assert(!std::is_copy_constructible_v<TlsRecordDecryptor>);
    static_assert(std::is_nothrow_move_constructible_v<TlsRecordDecryptor>);

public:
    void testCipherSuiteMetadataAndSecretValidation() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto aes128 = TlsCipherSuite{TlsCipherSuite::Aes128GcmSha256};
        const auto aes256 = TlsCipherSuite{TlsCipherSuite::Aes256GcmSha384};
        const auto chacha = TlsCipherSuite{TlsCipherSuite::ChaCha20Poly1305Sha256};
        REQUIRE_EQUAL(aes128.toRawValue(), uint16_t{0x1301U});
        REQUIRE_EQUAL(aes128.toString(), "TLS_AES_128_GCM_SHA256"_el);
        REQUIRE_EQUAL(aes128.hashAlgorithm(), HashAlgorithm::Sha2_256);
        REQUIRE_EQUAL(aes128.encryptionType(), SymmetricEncryptionType::Aes128Gcm);
        REQUIRE_EQUAL(aes256.hashAlgorithm(), HashAlgorithm::Sha2_384);
        REQUIRE_EQUAL(aes256.encryptionType(), SymmetricEncryptionType::Aes256Gcm);
        REQUIRE_EQUAL(chacha.encryptionType(), SymmetricEncryptionType::ChaCha20Poly1305);
        REQUIRE_EQUAL(TlsCipherSuite::all().size(), 3U);
        REQUIRE(TlsCipherSuite::fromRawValue(0x1303U).has_value());
        REQUIRE_FALSE(TlsCipherSuite::fromRawValue(0x1304U).has_value());

        auto consumed = ByteBlock{ByteLength{32U}, 0x31U};
        auto secret = TlsTrafficSecret::fromBytes(HashAlgorithm::Sha2_256, std::move(consumed));
        REQUIRE_EQUAL(consumed, ByteBlock{ByteLength{32U}});
        REQUIRE_EQUAL(secret.byteLength(), ByteLength{32U});
        REQUIRE_EQUAL(secret.hashAlgorithm(), HashAlgorithm::Sha2_256);
        secret.secureErase();
        REQUIRE(secret.isEmpty());
        REQUIRE_THROWS_AS(
            el::err::ParameterError, TlsTrafficSecret::fromBytes(HashAlgorithm::Sha2_384, ByteBlock{ByteLength{32U}}));
        const auto invalidSuite = TlsCipherSuite{static_cast<TlsCipherSuite::Value>(0xffffU)};
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            TlsRecordEncryptor(
                invalidSuite, TlsTrafficSecret::fromBytes(HashAlgorithm::Sha2_256, ByteBlock{ByteLength{32U}})));
    }

    void testRfc8448Aes128ApplicationRecord() {
        const auto applicationScope = ApplicationTestScope<>{};
        // RFC 8448 section 3, client application traffic secret and its first application_data record (sequence zero).
        const auto secret = bytesFromHex("9e40646ce79a7f9dc05af8889bce6552875afa0b06df0087f792ebb7c17504a5");
        const auto payload = bytesFromHex(
            "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f3031");
        const auto expected = bytesFromHex(
            "1703030043a23f7054b62c94d0affafe8228ba55cbefacea42f914aa66bcab3f2b9819a8a5b46b395bd54a9a20441e2b62974e1f5a"
            "6292a2977014bd1e3deae63aeebb21694915e4");
        auto encryptor =
            TlsRecordEncryptor{TlsCipherSuite::Aes128GcmSha256, makeSecret(TlsCipherSuite::Aes128GcmSha256, secret)};
        REQUIRE_EQUAL(encryptor.protect(TlsRecordContentType::ApplicationData, payload.span()), expected);

        auto decryptor =
            TlsRecordDecryptor{TlsCipherSuite::Aes128GcmSha256, makeSecret(TlsCipherSuite::Aes128GcmSha256, secret)};
        const auto plaintext = decryptor.unprotect(expected.span());
        REQUIRE_EQUAL(plaintext.type(), TlsRecordContentType::ApplicationData);
        REQUIRE_EQUAL(plaintext.content(), payload);
    }

    void testPinnedCompositionVectorsAndContentTypes() {
        const auto applicationScope = ApplicationTestScope<>{};
        // Independently generated with Node.js 24.4.1/OpenSSL 3.5.0 using RFC 8446 HKDF-Expand and AEAD APIs.
        const auto aes256Secret = sequentialBytes(48U);
        const auto aes256Expected = bytesFromHex("170303001748367d3f020db6f7590264e793780d8358314dd028d794");
        auto aes256 = TlsRecordEncryptor{
            TlsCipherSuite::Aes256GcmSha384, makeSecret(TlsCipherSuite::Aes256GcmSha384, aes256Secret)};
        REQUIRE_EQUAL(
            aes256.protect(TlsRecordContentType::ApplicationData, bytesFromHex("00010203").span(), ByteLength{2U}),
            aes256Expected);

        const auto chachaSecret = sequentialBytes(32U, true);
        const auto chachaExpected = bytesFromHex("1703030017e1b62da1a3de833b3c42efcd91250c2dd3cb593e760925");
        auto chacha = TlsRecordEncryptor{
            TlsCipherSuite::ChaCha20Poly1305Sha256, makeSecret(TlsCipherSuite::ChaCha20Poly1305Sha256, chachaSecret)};
        REQUIRE_EQUAL(
            chacha.protect(TlsRecordContentType::ApplicationData, bytesFromHex("00010203").span(), ByteLength{2U}),
            chachaExpected);

        WITH_CONTEXT(
            requireRoundTrip(TlsCipherSuite::Aes128GcmSha256, TlsRecordContentType::Alert, bytesFromHex("0100")));
        WITH_CONTEXT(requireRoundTrip(
            TlsCipherSuite::Aes256GcmSha384,
            TlsRecordContentType::Handshake,
            bytesFromHex("18000001"),
            ByteLength{31U}));
        WITH_CONTEXT(requireRoundTrip(
            TlsCipherSuite::ChaCha20Poly1305Sha256,
            TlsRecordContentType::ApplicationData,
            ByteBlock{},
            ByteLength{7U}));
    }

    void testSequencesDirectionsAndKeyUpdate() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto suite = TlsCipherSuite{TlsCipherSuite::Aes128GcmSha256};
        const auto secret = ByteBlock{ByteLength{32U}, 0xa7U};
        const auto payload = bytesFromHex("010203");
        auto sender = TlsRecordEncryptor{suite, makeSecret(suite, secret)};
        auto receiver = TlsRecordDecryptor{suite, makeSecret(suite, secret)};
        const auto first = sender.protect(TlsRecordContentType::ApplicationData, payload.span());
        const auto second = sender.protect(TlsRecordContentType::ApplicationData, payload.span());
        REQUIRE_FALSE(first == second);
        REQUIRE_EQUAL(receiver.unprotect(first.span()).content(), payload);
        REQUIRE_EQUAL(receiver.unprotect(second.span()).content(), payload);

        auto carrySender = TlsRecordEncryptor{suite, makeSecret(suite, secret)};
        auto carryReceiver = TlsRecordDecryptor{suite, makeSecret(suite, secret)};
        el::cryptology::impl::TlsRecordTestAccess{carrySender}.setCounters(0xffU, 0xffU);
        el::cryptology::impl::TlsRecordTestAccess{carryReceiver}.setCounters(0xffU, 0xffU);
        const auto carryRecord = carrySender.protect(TlsRecordContentType::ApplicationData, payload.span());
        REQUIRE_FALSE(carryRecord == first);
        REQUIRE_EQUAL(carryReceiver.unprotect(carryRecord.span()).content(), payload);
        REQUIRE_EQUAL(el::cryptology::impl::TlsRecordTestAccess{carrySender}.sequenceNumber(), uint64_t{0x100U});

        sender.updateApplicationTrafficKeys();
        receiver.updateApplicationTrafficKeys();
        const auto updated = sender.protect(TlsRecordContentType::ApplicationData, payload.span());
        REQUIRE_FALSE(updated == first);
        REQUIRE_EQUAL(receiver.unprotect(updated.span()).content(), payload);
        REQUIRE_EQUAL(el::cryptology::impl::TlsRecordTestAccess{sender}.sequenceNumber(), uint64_t{1U});
    }

    void testMalformedTamperedAndTerminalFailures() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto suite = TlsCipherSuite{TlsCipherSuite::Aes128GcmSha256};
        const auto secret = ByteBlock{ByteLength{32U}, 0x19U};
        const auto content = bytesFromHex("01020304");
        auto sender = TlsRecordEncryptor{suite, makeSecret(suite, secret)};
        const auto record = sender.protect(TlsRecordContentType::ApplicationData, content.span());

        auto truncated = TlsRecordDecryptor{suite, makeSecret(suite, secret)};
        requireRecordError(TlsRecordErrorCategory::DecodeError, [&] {
            static_cast<void>(truncated.unprotect(record.span().first(4U)));
        });
        REQUIRE(truncated.isEmpty());

        auto extraBytes = el::mem::ByteBlockEditor{record};
        extraBytes.append(el::Byte{});
        auto extra = TlsRecordDecryptor{suite, makeSecret(suite, secret)};
        requireRecordError(
            TlsRecordErrorCategory::DecodeError, [&] { static_cast<void>(extra.unprotect(extraBytes.span())); });

        auto oversizedBytes = el::mem::ByteBlockEditor{record};
        oversizedBytes.overwrite(el::ByteIndex{3U}, ByteBlock({0x41U, 0x01U}).span());
        auto oversized = TlsRecordDecryptor{suite, makeSecret(suite, secret)};
        requireRecordError(TlsRecordErrorCategory::RecordOverflow, [&] {
            static_cast<void>(oversized.unprotect(oversizedBytes.span()));
        });

        auto invalidHeaderBytes = el::mem::ByteBlockEditor{record};
        invalidHeaderBytes.overwrite(el::ByteIndex{0U}, ByteBlock({22U}).span());
        auto invalidHeader = TlsRecordDecryptor{suite, makeSecret(suite, secret)};
        requireRecordError(TlsRecordErrorCategory::UnexpectedMessage, [&] {
            static_cast<void>(invalidHeader.unprotect(invalidHeaderBytes.span()));
        });
        REQUIRE(invalidHeader.isEmpty());

        auto tamperedBytes = el::mem::ByteBlockEditor{record};
        tamperedBytes.xorWith(el::ByteRange{el::ByteIndex{6U}, ByteLength{1U}}, ByteBlock({0x80U}).span());
        auto tampered = TlsRecordDecryptor{suite, makeSecret(suite, secret)};
        requireRecordError(
            TlsRecordErrorCategory::BadRecordMac, [&] { static_cast<void>(tampered.unprotect(tamperedBytes.span())); });
        REQUIRE(tampered.isEmpty());
        REQUIRE_THROWS_AS(el::err::LogicError, tampered.unprotect(record.span()));
    }

    void testBoundsUsageLimitsAndLocalErrors() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto suite = TlsCipherSuite{TlsCipherSuite::Aes128GcmSha256};
        const auto secret = ByteBlock{ByteLength{32U}, 0x44U};
        auto sender = TlsRecordEncryptor{suite, makeSecret(suite, secret)};
        REQUIRE_THROWS_AS(el::err::ParameterError, sender.protect(TlsRecordContentType::Alert, ByteBlock{}.span()));
        REQUIRE_FALSE(sender.isEmpty());
        REQUIRE_EQUAL(el::cryptology::impl::TlsRecordTestAccess{sender}.sequenceNumber(), uint64_t{0U});

        const auto maximumContent = ByteBlock{ByteLength{uint16_t{1U} << 14U}, 0x61U};
        const auto maximumRecord = sender.protect(TlsRecordContentType::ApplicationData, maximumContent.span());
        REQUIRE_EQUAL(maximumRecord.length(), ByteLength{5U + (uint16_t{1U} << 14U) + 1U + 16U});
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            sender.protect(TlsRecordContentType::ApplicationData, maximumContent.span(), ByteLength{1U}));
        REQUIRE_FALSE(sender.isEmpty());

        el::cryptology::impl::TlsRecordTestAccess{sender}.setCounters(uint64_t{1U} << 24U, uint64_t{1U} << 24U);
        REQUIRE(sender.isKeyUpdateRequired());
        el::cryptology::impl::TlsRecordTestAccess{sender}.setCounters(23'726'566U, 23'726'566U);
        requireRecordError(TlsRecordErrorCategory::KeyUsageExhausted, [&] {
            static_cast<void>(sender.protect(TlsRecordContentType::ApplicationData, ByteBlock{}.span()));
        });
        REQUIRE(sender.isEmpty());

        const auto chachaSuite = TlsCipherSuite{TlsCipherSuite::ChaCha20Poly1305Sha256};
        auto chacha = TlsRecordEncryptor{chachaSuite, makeSecret(chachaSuite, ByteBlock{ByteLength{32U}, 0x55U})};
        el::cryptology::impl::TlsRecordTestAccess{chacha}.setCounters(
            std::numeric_limits<uint64_t>::max() - 1U, std::numeric_limits<uint64_t>::max() - 1U);
        REQUIRE(chacha.isKeyUpdateRequired());
        static_cast<void>(chacha.protect(TlsRecordContentType::ApplicationData, ByteBlock{}.span()));
        requireRecordError(TlsRecordErrorCategory::KeyUsageExhausted, [&] {
            static_cast<void>(chacha.protect(TlsRecordContentType::ApplicationData, ByteBlock{}.span()));
        });
        REQUIRE(chacha.isEmpty());
    }
};
