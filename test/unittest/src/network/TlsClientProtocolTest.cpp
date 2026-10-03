// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/Byte.hpp>
#include <erbsland/ByteArray.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/KeyAgreementAlgorithm.hpp>
#include <erbsland/KeyAgreementPrivateKey.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocol.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolTestAccess.hpp>
#include <erbsland/network/impl/tls/TlsExtensionRules.hpp>
#include <erbsland/network/impl/tls/TlsKeyShare.hpp>
#include <erbsland/network/impl/tls/TlsProtocolError.hpp>
#include <erbsland/network/impl/tls/TlsWireReader.hpp>
#include <erbsland/network/impl/tls/TlsWireWriter.hpp>
#include <erbsland/NetworkSendStatus.hpp>
#include <erbsland/String.hpp>
#include <erbsland/Timestamp.hpp>
#include <erbsland/TlsRecordContentType.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/X509CertificateBundle.hpp>
#include <erbsland/X509TlsCertificatePolicy.hpp>

#include <cstdint>
#include <utility>
#include <vector>

using namespace el::cryptology;
using namespace el::network;
using namespace el::text::literals;
using el::mem::Byte;
using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;
using el::unit::ByteCount;
using el::unit::ByteIndex;

TESTED_TARGETS(TlsClientHelloBuilder TlsClientProtocolOptions TlsClientProtocolState TlsClientProtocol)
class TlsClientProtocolTest final : public el::UnitTest {
    using TlsClientHelloBuilder = el::network::impl::TlsClientHelloBuilder;
    using TlsClientProtocol = el::network::impl::TlsClientProtocol;
    using TlsClientProtocolCheckpoint = el::network::impl::TlsClientProtocolCheckpoint;
    using TlsClientProtocolOptions = el::network::impl::TlsClientProtocolOptions;
    using TlsClientProtocolState = el::network::impl::TlsClientProtocolState;
    using TlsClientProtocolTestAccess = el::network::impl::TlsClientProtocolTestAccess;
    using TlsWireWriter = el::network::impl::TlsWireWriter;

private:
    [[nodiscard]] static auto retryHello(
        const ByteBlock &sessionId, const ByteBlock &cookie, const std::optional<uint16_t> group = {}) -> ByteBlock {
        auto extensions = TlsWireWriter{};
        extensions.writeU16(43U);
        extensions.writeVector16(ByteBlock({3U, 4U}).span());
        if (!cookie.isEmpty()) {
            auto cookieBody = TlsWireWriter{};
            cookieBody.writeVector16(cookie.span());
            extensions.writeU16(44U);
            extensions.writeVector16(cookieBody.finish().span());
        }
        if (group.has_value()) {
            auto groupBody = TlsWireWriter{};
            groupBody.writeU16(*group);
            extensions.writeU16(51U);
            extensions.writeVector16(groupBody.finish().span());
        }
        auto body = TlsWireWriter{};
        body.writeU16(0x0303U);
        body.writeBytes(el::network::impl::tls_key_share::helloRetryRequestRandom().span());
        body.writeVector8(sessionId.span());
        body.writeU16(0x1301U);
        body.writeU8(0U);
        body.writeVector16(extensions.finish().span());
        return handshake(2U, body.finish());
    }

    [[nodiscard]] static auto clientHelloExtension(const ByteBlock &message, const uint16_t type) -> ByteBlock {
        auto reader = el::network::impl::TlsWireReader{message.span().subspan(4U)};
        static_cast<void>(reader.readBytes(34U));
        static_cast<void>(reader.readVector8());
        static_cast<void>(reader.readVector16());
        static_cast<void>(reader.readVector8());
        auto extensions = el::network::impl::TlsWireReader{reader.readVector16()};
        reader.requireEnd();
        while (!extensions.isAtEnd()) {
            const auto extensionType = extensions.readU16();
            const auto body = extensions.readVector16();
            if (extensionType == type) {
                return ByteBlock::fromSpan(body);
            }
        }
        return {};
    }

    [[nodiscard]] static auto options(el::StringList alpn = {}) -> TlsClientProtocolOptions {
        return TlsClientProtocolOptions{
            Host::fromStringOrThrow("server.example"_el),
            X509TlsCertificatePolicy{X509CertificateBundle{}},
            el::time::Timestamp::now(),
            std::move(alpn)};
    }

    [[nodiscard]] static auto privateKey(const uint8_t value) -> KeyAgreementPrivateKey {
        return KeyAgreementPrivateKey::fromBytes(
            KeyAgreementAlgorithm::X25519, ByteBlock{ByteCount{32U}, value}.span());
    }

    [[nodiscard]] static auto handshake(const uint8_t type, const ByteBlock &body) -> ByteBlock {
        auto writer = TlsWireWriter{};
        writer.writeU8(type);
        writer.writeU24(static_cast<uint32_t>(body.length().toSizeT()));
        writer.writeBytes(body.span());
        return writer.finish();
    }

    [[nodiscard]] static auto plaintextRecord(const uint8_t type, const ByteBlock &content) -> ByteBlock {
        auto writer = TlsWireWriter{};
        writer.writeU8(type);
        writer.writeU16(0x0303U);
        writer.writeU16(static_cast<uint16_t>(content.length().toSizeT()));
        writer.writeBytes(content.span());
        return writer.finish();
    }

    [[nodiscard]] static auto certificateMessage(const X509Certificate &target, const X509Certificate &intermediate)
        -> ByteBlock {
        auto certificateList = TlsWireWriter{};
        certificateList.writeVector24(target.toDer().span());
        certificateList.writeVector16({});
        certificateList.writeVector24(intermediate.toDer().span());
        certificateList.writeVector16({});
        const auto certificateListBytes = certificateList.finish();
        auto body = TlsWireWriter{};
        body.writeVector8({});
        body.writeVector24(certificateListBytes.span());
        return handshake(11U, body.finish());
    }

    [[nodiscard]] static auto serverHello(
        const ByteBlock &sessionId, const KeyAgreementPrivateKey &serverPrivate, const uint16_t suite = 0x1301U)
        -> ByteBlock {
        auto extensions = TlsWireWriter{};
        const auto versionBody = ByteBlock({0x03U, 0x04U});
        extensions.writeU16(43U);
        extensions.writeVector16(versionBody.span());

        auto keyShare = TlsWireWriter{};
        keyShare.writeU16(0x001dU);
        const auto publicKey = serverPrivate.publicKey();
        keyShare.writeVector16(publicKey.span());
        const auto keyShareBody = keyShare.finish();
        extensions.writeU16(51U);
        extensions.writeVector16(keyShareBody.span());
        const auto extensionBytes = extensions.finish();

        auto body = TlsWireWriter{};
        body.writeU16(0x0303U);
        body.writeBytes(ByteBlock{ByteCount{32U}, 0x33U}.span());
        body.writeVector8(sessionId.span());
        body.writeU16(suite);
        body.writeU8(0U);
        body.writeVector16(extensionBytes.span());
        return handshake(2U, body.finish());
    }

    static void feedFragmented(TlsClientProtocol &protocol, const ByteBlock &record) {
        for (const auto byte : record.span()) {
            protocol.feedTransport(ByteBlock({byte}).span());
        }
    }

public:
    void testCookieOnlyRetryPreservesShareAndFragmentsHello() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto protocol = TlsClientProtocol{options()};
        const auto sessionId = ByteBlock{ByteCount{32U}, 0x22U};
        TlsClientProtocolTestAccess{protocol}.start(ByteBlock{ByteCount{32U}, 0x11U}, sessionId, privateKey(0x44U));
        const auto firstRecord = protocol.takeTransportOutput();
        REQUIRE(firstRecord.has_value());
        const auto firstHello = ByteBlock::fromSpan(firstRecord->span().subspan(5U));
        const auto cookie = ByteBlock{ByteCount{60U * 1024U}, 0x55U};
        const auto retry = retryHello(sessionId, cookie);
        auto remaining = retry.span();
        while (!remaining.empty()) {
            const auto length = std::min(remaining.size(), std::size_t{16384U});
            protocol.feedTransport(plaintextRecord(22U, ByteBlock::fromSpan(remaining.first(length))).span());
            remaining = remaining.subspan(length);
        }
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Handshaking);
        const auto ccs = protocol.takeTransportOutput();
        REQUIRE(ccs.has_value());
        REQUIRE_EQUAL(ccs->get(ByteIndex{0U}), Byte{20U});
        auto secondHello = ByteBlockEditor{};
        auto fragments = 0U;
        while (const auto record = protocol.takeTransportOutput()) {
            REQUIRE(record->length() <= ByteCount{16389U});
            REQUIRE_EQUAL(record->get(ByteIndex{0U}), Byte{22U});
            secondHello.append(record->span().subspan(5U));
            ++fragments;
        }
        REQUIRE(fragments > 1U);
        REQUIRE_EQUAL(clientHelloExtension(ByteBlock{secondHello}, 51U), clientHelloExtension(firstHello, 51U));
        const auto echoedCookie = clientHelloExtension(ByteBlock{secondHello}, 44U);
        auto cookieReader = el::network::impl::TlsWireReader{echoedCookie.span()};
        REQUIRE_EQUAL(ByteBlock::fromSpan(cookieReader.readVector16()), cookie);
        cookieReader.requireEnd();
        protocol.feedTransport(plaintextRecord(22U, retryHello(sessionId, ByteBlock({1U}))).span());
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Failed);
        REQUIRE_EQUAL(*protocol.failureAlert(), TlsAlertDescription::UnexpectedMessage);
    }

    void testRetryRejectsAlreadyOfferedAndUnknownGroups() {
        const auto applicationScope = ApplicationTestScope<>{};
        for (const auto group : {uint16_t{0x001dU}, uint16_t{0xffffU}}) {
            auto protocol = TlsClientProtocol{options()};
            const auto sessionId = ByteBlock{ByteCount{32U}, 0x22U};
            TlsClientProtocolTestAccess{protocol}.start(ByteBlock{ByteCount{32U}, 0x11U}, sessionId, privateKey(0x44U));
            static_cast<void>(protocol.takeTransportOutput());
            protocol.feedTransport(plaintextRecord(22U, retryHello(sessionId, {}, group)).span());
            REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Failed);
            REQUIRE_EQUAL(*protocol.failureAlert(), TlsAlertDescription::IllegalParameter);
        }
    }

    void testUnknownExtensionContexts() {
        using Error = el::network::impl::TlsProtocolError;
        el::network::impl::tls_extension_rules::requireIgnorableCertificateRequest(0xffffU);
        el::network::impl::tls_extension_rules::requireIgnorableSessionTicket(0xffffU);
        REQUIRE_THROWS_AS(Error, el::network::impl::tls_extension_rules::requireIgnorableCertificateRequest(51U));
        REQUIRE_THROWS_AS(Error, el::network::impl::tls_extension_rules::requireIgnorableSessionTicket(13U));
    }

    void testAuthenticatedCoreApplicationKeyUpdateAndClose() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto protocol = TlsClientProtocol{options(el::StringList{"h2"_el})};
        auto access = TlsClientProtocolTestAccess{protocol};
        const auto clientRandom = ByteBlock{ByteCount{32U}, 0x11U};
        const auto sessionId = ByteBlock{ByteCount{32U}, 0x22U};
        access.start(clientRandom, sessionId, privateKey(0x44U));
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Handshaking);

        const auto clientHelloRecord = protocol.takeTransportOutput();
        REQUIRE(clientHelloRecord.has_value());
        REQUIRE_EQUAL(clientHelloRecord->get(ByteIndex{0U}), Byte{22U});
        REQUIRE_EQUAL(clientHelloRecord->get(ByteIndex{1U}), Byte{0x03U});
        REQUIRE_EQUAL(clientHelloRecord->get(ByteIndex{2U}), Byte{0x01U});

        auto serverPrivate = privateKey(0x55U);
        feedFragmented(protocol, plaintextRecord(22U, serverHello(sessionId, serverPrivate)));
        REQUIRE(protocol.cipherSuite().has_value());
        REQUIRE_EQUAL(*protocol.cipherSuite(), TlsCipherSuite::Aes128GcmSha256);

        auto serverHandshakeSender = access.serverHandshakeEncryptor();
        auto clientHandshakeReceiver = access.clientHandshakeDecryptor();
        const auto encryptedExtensions = handshake(8U, ByteBlock({0x00U, 0x00U}));
        protocol.feedTransport(
            serverHandshakeSender.protect(TlsRecordContentType::Handshake, encryptedExtensions.span()).span());
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Handshaking);
        REQUIRE_EQUAL(protocol.checkpoint(), TlsClientProtocolCheckpoint::PeerHello);
        protocol.resume();
        REQUIRE_FALSE(protocol.hasCheckpoint());

        // Certificate path construction and CertificateVerify primitives have dedicated exhaustive suites. This test
        // retains exact placeholder encodings in the transcript while concentrating on protocol/key transition order.
        const auto certificate = ByteBlock({0x0bU, 0x00U, 0x00U, 0x04U, 0x00U, 0x00U, 0x00U, 0x00U});
        const auto certificateVerify = ByteBlock({0x0fU, 0x00U, 0x00U, 0x04U, 0x08U, 0x04U, 0x00U, 0x00U});
        access.acceptServerAuthentication(certificate.span(), certificateVerify.span());
        REQUIRE_EQUAL(protocol.checkpoint(), TlsClientProtocolCheckpoint::PeerAuthenticated);
        protocol.resume();
        const auto serverFinished = access.serverFinishedMessage();
        protocol.feedTransport(
            serverHandshakeSender.protect(TlsRecordContentType::Handshake, serverFinished.span()).span());
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Established);
        REQUIRE_EQUAL(protocol.checkpoint(), TlsClientProtocolCheckpoint::HandshakeCompleted);
        const auto checkpointPayload = ByteBlock({'x'});
        REQUIRE_EQUAL(protocol.sendApplication(checkpointPayload.span()), NetworkSendStatus::Accepted);
        protocol.resume();

        const auto clientCcs = protocol.takeTransportOutput();
        REQUIRE(clientCcs.has_value());
        REQUIRE_EQUAL(clientCcs->get(ByteIndex{0U}), Byte{20U});
        const auto clientFinishedRecord = protocol.takeTransportOutput();
        REQUIRE(clientFinishedRecord.has_value());
        const auto clientFinished = clientHandshakeReceiver.unprotect(clientFinishedRecord->span());
        REQUIRE_EQUAL(clientFinished.type(), TlsRecordContentType::Handshake);
        REQUIRE_EQUAL(clientFinished.content().get(ByteIndex{0U}), Byte{20U});

        auto clientApplicationReceiver = access.clientApplicationDecryptor();
        const auto checkpointApplicationRecord = protocol.takeTransportOutput();
        REQUIRE(checkpointApplicationRecord.has_value());
        REQUIRE_EQUAL(
            clientApplicationReceiver.unprotect(checkpointApplicationRecord->span()).content(), checkpointPayload);
        const auto clientPayload = ByteBlock({'p', 'i', 'n', 'g'});
        REQUIRE_EQUAL(protocol.sendApplication(clientPayload.span()), NetworkSendStatus::Accepted);
        const auto protectedClientData = protocol.takeTransportOutput();
        REQUIRE(protectedClientData.has_value());
        REQUIRE_EQUAL(clientApplicationReceiver.unprotect(protectedClientData->span()).content(), clientPayload);

        auto serverApplicationSender = access.serverApplicationEncryptor();
        auto ticketExtensions = TlsWireWriter{};
        ticketExtensions.writeU16(0xffffU);
        ticketExtensions.writeVector16(ByteBlock({1U, 2U, 3U}).span());
        auto ticketBody = TlsWireWriter{};
        ticketBody.writeBytes(ByteBlock({0U, 0U, 0U, 60U, 0U, 0U, 0U, 1U}).span());
        ticketBody.writeVector8({});
        ticketBody.writeVector16(ByteBlock({1U}).span());
        ticketBody.writeVector16(ticketExtensions.finish().span());
        const auto ticket = handshake(4U, ticketBody.finish());
        protocol.feedTransport(serverApplicationSender.protect(TlsRecordContentType::Handshake, ticket.span()).span());
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Established);
        const auto serverPayload = ByteBlock({'p', 'o', 'n', 'g'});
        protocol.feedTransport(
            serverApplicationSender.protect(TlsRecordContentType::ApplicationData, serverPayload.span()).span());
        REQUIRE_EQUAL(*protocol.takeApplicationData(), serverPayload);
        protocol.feedTransport(serverApplicationSender.protect(TlsRecordContentType::ApplicationData, {}).span());
        REQUIRE_FALSE(protocol.hasApplicationData());

        // RFC 8446 section 4.6.3: peer request is protected with old keys; both directions advance only after their
        // respective KeyUpdate record.
        const auto requestedUpdate = ByteBlock({24U, 0U, 0U, 1U, 1U});
        protocol.feedTransport(
            serverApplicationSender.protect(TlsRecordContentType::Handshake, requestedUpdate.span()).span());
        serverApplicationSender.updateApplicationTrafficKeys();
        const auto updateResponseRecord = protocol.takeTransportOutput();
        REQUIRE(updateResponseRecord.has_value());
        const auto updateResponse = clientApplicationReceiver.unprotect(updateResponseRecord->span());
        REQUIRE_EQUAL(updateResponse.content(), ByteBlock({24U, 0U, 0U, 1U, 0U}));
        clientApplicationReceiver.updateApplicationTrafficKeys();

        const auto updatedPayload = ByteBlock({'n', 'e', 'w'});
        protocol.feedTransport(
            serverApplicationSender.protect(TlsRecordContentType::ApplicationData, updatedPayload.span()).span());
        REQUIRE_EQUAL(*protocol.takeApplicationData(), updatedPayload);

        protocol.feedTransport(
            serverApplicationSender.protect(TlsRecordContentType::Alert, ByteBlock({1U, 90U}).span()).span());
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Established);
        REQUIRE_FALSE(protocol.failureAlert().has_value());

        protocol.close();
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Closing);
        const auto localCloseRecord = protocol.takeTransportOutput();
        REQUIRE(localCloseRecord.has_value());
        const auto localClose = clientApplicationReceiver.unprotect(localCloseRecord->span());
        REQUIRE_EQUAL(localClose.type(), TlsRecordContentType::Alert);
        REQUIRE_EQUAL(localClose.content(), ByteBlock({1U, 0U}));

        protocol.feedTransport(
            serverApplicationSender.protect(TlsRecordContentType::Alert, ByteBlock({1U, 0U}).span()).span());
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Closed);
    }

    void testMalformedServerHelloTimeoutAndTruncation() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto sessionId = ByteBlock{ByteCount{32U}, 0x22U};
        auto protocol = TlsClientProtocol{options()};
        auto access = TlsClientProtocolTestAccess{protocol};
        access.start(ByteBlock{ByteCount{32U}, 0x11U}, sessionId, privateKey(0x44U));
        [[maybe_unused]] const auto initialOutput = protocol.takeTransportOutput();

        auto wrongSession = ByteBlock{ByteCount{32U}, 0x23U};
        auto serverPrivate = privateKey(0x55U);
        protocol.feedTransport(plaintextRecord(22U, serverHello(wrongSession, serverPrivate)).span());
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Failed);
        REQUIRE_EQUAL(*protocol.failureAlert(), TlsAlertDescription::IllegalParameter);

        auto crossedBoundary = TlsClientProtocol{options()};
        auto crossedBoundaryAccess = TlsClientProtocolTestAccess{crossedBoundary};
        crossedBoundaryAccess.start(ByteBlock{ByteCount{32U}, 0x11U}, sessionId, privateKey(0x44U));
        [[maybe_unused]] const auto crossedBoundaryOutput = crossedBoundary.takeTransportOutput();
        auto coalesced = ByteBlockEditor{};
        coalesced.append(serverHello(sessionId, serverPrivate));
        coalesced.append(Byte{0x08U});
        crossedBoundary.feedTransport(plaintextRecord(22U, ByteBlock{coalesced}).span());
        REQUIRE_EQUAL(crossedBoundary.state(), TlsClientProtocolState::Failed);
        REQUIRE_EQUAL(*crossedBoundary.failureAlert(), TlsAlertDescription::UnexpectedMessage);

        auto zeroHandshake = TlsClientProtocol{options()};
        zeroHandshake.start();
        [[maybe_unused]] const auto zeroHandshakeOutput = zeroHandshake.takeTransportOutput();
        zeroHandshake.feedTransport(plaintextRecord(22U, {}).span());
        REQUIRE_EQUAL(zeroHandshake.state(), TlsClientProtocolState::Failed);
        REQUIRE_EQUAL(*zeroHandshake.failureAlert(), TlsAlertDescription::UnexpectedMessage);

        auto timedOut = TlsClientProtocol{options()};
        timedOut.start();
        timedOut.timeout();
        REQUIRE_EQUAL(timedOut.state(), TlsClientProtocolState::Failed);

        auto truncated = TlsClientProtocol{options()};
        truncated.start();
        truncated.transportClosed();
        REQUIRE_EQUAL(truncated.state(), TlsClientProtocolState::Failed);
        REQUIRE_EQUAL(*truncated.failureAlert(), TlsAlertDescription::UnexpectedMessage);
    }

    void testCertificateMessageUsesExplicitAnchorPolicy() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto bundle = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/cryptology/x509/server_authentication.pem")});
        const auto &certificates = bundle.certificates().toRawValue();
        const auto root = certificates[0U];
        const auto intermediate = certificates[1U];
        const auto target = certificates[2U];
        auto protocol = TlsClientProtocol{TlsClientProtocolOptions{
            Host::fromStringOrThrow("server.example.test"_el),
            X509TlsCertificatePolicy{X509CertificateBundle{el::util::List<X509Certificate>{root}}},
            el::time::Timestamp::now()}};
        auto access = TlsClientProtocolTestAccess{protocol};
        const auto sessionId = ByteBlock{ByteCount{32U}, 0x22U};
        access.start(ByteBlock{ByteCount{32U}, 0x11U}, sessionId, privateKey(0x44U));
        [[maybe_unused]] const auto clientHelloOutput = protocol.takeTransportOutput();

        auto serverPrivate = privateKey(0x55U);
        protocol.feedTransport(plaintextRecord(22U, serverHello(sessionId, serverPrivate)).span());
        auto serverHandshakeSender = access.serverHandshakeEncryptor();
        const auto encryptedExtensions = handshake(8U, ByteBlock({0U, 0U}));
        const auto encryptedExtensionsRecord =
            serverHandshakeSender.protect(TlsRecordContentType::Handshake, encryptedExtensions.span());
        const auto certificate = certificateMessage(target, intermediate);
        const auto certificateRecord =
            serverHandshakeSender.protect(TlsRecordContentType::Handshake, certificate.span());
        auto coalescedRecords = ByteBlockEditor{};
        coalescedRecords.append(encryptedExtensionsRecord);
        coalescedRecords.append(certificateRecord);
        protocol.feedTransport(coalescedRecords.span());
        REQUIRE_EQUAL(protocol.checkpoint(), TlsClientProtocolCheckpoint::PeerHello);
        REQUIRE(protocol.validatedPath().isEmpty());
        protocol.resume();
        REQUIRE_EQUAL(protocol.checkpoint(), TlsClientProtocolCheckpoint::CertificateValidation);
        protocol.resume();
        REQUIRE(protocol.validatedPath().isEmpty());
        REQUIRE(protocol.pendingCertificateValidation().has_value());
        protocol.completeCertificateValidation(protocol.pendingCertificateValidation()->execute());
        REQUIRE_EQUAL(protocol.validatedPath().count(), el::unit::ItemCount{3U});
        REQUIRE_EQUAL(protocol.validatedPath().toRawValue()[0U].toDer(), target.toDer());

        // Signature primitives and every supported TLS scheme have dedicated tests; retain one exact placeholder here
        // so this integration test can continue from the real certificate parser/policy boundary.
        const auto certificateVerify = ByteBlock({15U, 0U, 0U, 4U, 8U, 4U, 0U, 0U});
        access.acceptCertificateVerify(certificateVerify.span());
        REQUIRE_EQUAL(protocol.checkpoint(), TlsClientProtocolCheckpoint::PeerAuthenticated);
        protocol.resume();
        const auto finished = access.serverFinishedMessage();
        protocol.feedTransport(serverHandshakeSender.protect(TlsRecordContentType::Handshake, finished.span()).span());
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Established);
        REQUIRE_EQUAL(protocol.checkpoint(), TlsClientProtocolCheckpoint::HandshakeCompleted);
        protocol.resume();
    }

    void testCheckpointAbortDiscardsQueuedNextFlight() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto protocol = TlsClientProtocol{options()};
        auto access = TlsClientProtocolTestAccess{protocol};
        const auto sessionId = ByteBlock{ByteCount{32U}, 0x22U};
        access.start(ByteBlock{ByteCount{32U}, 0x11U}, sessionId, privateKey(0x44U));
        [[maybe_unused]] const auto clientHello = protocol.takeTransportOutput();
        auto serverPrivate = privateKey(0x55U);
        protocol.feedTransport(plaintextRecord(22U, serverHello(sessionId, serverPrivate)).span());
        auto serverHandshakeSender = access.serverHandshakeEncryptor();
        const auto encryptedExtensions = handshake(8U, ByteBlock({0U, 0U}));
        protocol.feedTransport(
            serverHandshakeSender.protect(TlsRecordContentType::Handshake, encryptedExtensions.span()).span());
        REQUIRE_EQUAL(protocol.checkpoint(), TlsClientProtocolCheckpoint::PeerHello);
        protocol.resume();
        access.acceptServerAuthentication(
            ByteBlock({11U, 0U, 0U, 4U, 0U, 0U, 0U, 0U}).span(), ByteBlock({15U, 0U, 0U, 4U, 8U, 4U, 0U, 0U}).span());
        protocol.resume();
        const auto serverFinished = access.serverFinishedMessage();
        protocol.feedTransport(
            serverHandshakeSender.protect(TlsRecordContentType::Handshake, serverFinished.span()).span());
        REQUIRE_EQUAL(protocol.checkpoint(), TlsClientProtocolCheckpoint::HandshakeCompleted);
        REQUIRE(protocol.hasTransportOutput());
        protocol.abort();
        REQUIRE_FALSE(protocol.hasTransportOutput());
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Closed);
    }
};
