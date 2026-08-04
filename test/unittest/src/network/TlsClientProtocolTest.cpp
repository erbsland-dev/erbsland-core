// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/cryptology/keys/KeyAgreementAlgorithm.hpp>
#include <erbsland/cryptology/keys/KeyAgreementPrivateKey.hpp>
#include <erbsland/cryptology/tls_record/TlsRecordContentType.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/cryptology/x509/X509ServerCertificatePolicy.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteArray.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/network/Host.hpp>
#include <erbsland/network/impl/TlsClientProtocol.hpp>
#include <erbsland/network/impl/TlsClientProtocolTestAccess.hpp>
#include <erbsland/network/impl/TlsWireWriter.hpp>
#include <erbsland/network/source/NetworkSendStatus.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/List.hpp>

#include <cstdint>
#include <utility>
#include <vector>

using namespace el::cryptology;
using namespace el::network;
using namespace el::network::impl;
using namespace el::text::literals;
using el::mem::Byte;
using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;
using el::unit::ByteIndex;
using el::unit::ByteLength;

TESTED_TARGETS(TlsClientHelloBuilder TlsClientProtocolOptions TlsClientProtocolState TlsClientProtocol)
class TlsClientProtocolTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto options(std::vector<ByteBlock> alpn = {}) -> TlsClientProtocolOptions {
        return TlsClientProtocolOptions{
            Host::fromStringOrThrow("server.example"_el),
            X509ServerCertificatePolicy{X509CertificateBundle{}},
            el::time::DateTime::now(),
            std::move(alpn)};
    }

    [[nodiscard]] static auto privateKey(const uint8_t value) -> KeyAgreementPrivateKey {
        return KeyAgreementPrivateKey::fromBytes(
            KeyAgreementAlgorithm::X25519, ByteBlock{ByteLength{32U}, value}.span());
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
        body.writeBytes(ByteBlock{ByteLength{32U}, 0x33U}.span());
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
    void testAuthenticatedCoreApplicationKeyUpdateAndClose() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto protocol = TlsClientProtocol{options({ByteBlock({'h', '2'})})};
        auto access = TlsClientProtocolTestAccess{protocol};
        const auto clientRandom = ByteBlock{ByteLength{32U}, 0x11U};
        const auto sessionId = ByteBlock{ByteLength{32U}, 0x22U};
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
        const auto sessionId = ByteBlock{ByteLength{32U}, 0x22U};
        auto protocol = TlsClientProtocol{options()};
        auto access = TlsClientProtocolTestAccess{protocol};
        access.start(ByteBlock{ByteLength{32U}, 0x11U}, sessionId, privateKey(0x44U));
        [[maybe_unused]] const auto initialOutput = protocol.takeTransportOutput();

        auto wrongSession = ByteBlock{ByteLength{32U}, 0x23U};
        auto serverPrivate = privateKey(0x55U);
        protocol.feedTransport(plaintextRecord(22U, serverHello(wrongSession, serverPrivate)).span());
        REQUIRE_EQUAL(protocol.state(), TlsClientProtocolState::Failed);
        REQUIRE_EQUAL(*protocol.failureAlert(), TlsAlertDescription::IllegalParameter);

        auto crossedBoundary = TlsClientProtocol{options()};
        auto crossedBoundaryAccess = TlsClientProtocolTestAccess{crossedBoundary};
        crossedBoundaryAccess.start(ByteBlock{ByteLength{32U}, 0x11U}, sessionId, privateKey(0x44U));
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
            X509ServerCertificatePolicy{X509CertificateBundle{el::util::List<X509Certificate>{root}}},
            el::time::DateTime::now()}};
        auto access = TlsClientProtocolTestAccess{protocol};
        const auto sessionId = ByteBlock{ByteLength{32U}, 0x22U};
        access.start(ByteBlock{ByteLength{32U}, 0x11U}, sessionId, privateKey(0x44U));
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
        const auto sessionId = ByteBlock{ByteLength{32U}, 0x22U};
        access.start(ByteBlock{ByteLength{32U}, 0x11U}, sessionId, privateKey(0x44U));
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
