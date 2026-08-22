// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/cryptology/keys/KeyAgreementAlgorithm.hpp>
#include <erbsland/cryptology/keys/KeyAgreementPrivateKey.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/tls/TlsServerIdentity.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/cryptology/x509/X509ServerCertificatePolicy.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/network/Host.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocol.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolOptions.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolTestAccess.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocol.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocolTestAccess.hpp>
#include <erbsland/network/impl/tls/TlsAlpnProtocol.hpp>
#include <erbsland/network/impl/tls/TlsWireWriter.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <format>
#include <memory>
#include <utility>
#include <vector>

using namespace el::cryptology;
using namespace el::network;
using namespace el::network::impl;
using namespace el::text::literals;
using el::mem::ByteBlock;
using el::unit::ByteLength;

TESTED_TARGETS(TlsServerProtocol TlsServerProtocolOptions TlsServerProtocolState TlsServerProtocolCheckpoint)
class TlsServerProtocolTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto readText(const char *path) -> el::text::String {
        return el::text::String{el::unittest::fh::readDataText(path)};
    }

    [[nodiscard]] static auto identity(
        const char *certificatePath = "data/network/tls-interop/server.pem",
        const char *keyPath = "data/network/tls-interop/server-key.pem") -> TlsServerIdentityConstPtr {
        return std::make_shared<const TlsServerIdentity>(
            X509CertificateBundle::fromPemOrThrow(readText(certificatePath)),
            SigningPrivateKey::fromPemOrThrow(readText(keyPath)));
    }

    [[nodiscard]] static auto clientOptions(
        std::vector<el::text::String> alpn = {}, const char *trustPath = "data/network/tls-interop/ca.pem")
        -> TlsClientProtocolOptions {
        return clientOptionsForHost("localhost"_el, std::move(alpn), trustPath);
    }

    [[nodiscard]] static auto clientOptionsForHost(
        const el::text::String &host,
        std::vector<el::text::String> alpn = {},
        const char *trustPath = "data/network/tls-interop/ca.pem") -> TlsClientProtocolOptions {
        return TlsClientProtocolOptions{
            Host::fromStringOrThrow(host),
            X509ServerCertificatePolicy{X509CertificateBundle::fromPemOrThrow(readText(trustPath))},
            el::time::DateTime::now(),
            std::move(alpn)};
    }

    [[nodiscard]] static auto privateKey(const uint8_t value) -> KeyAgreementPrivateKey {
        return KeyAgreementPrivateKey::fromBytes(
            KeyAgreementAlgorithm::X25519, ByteBlock{ByteLength{32U}, value}.span());
    }

    static auto transferClientOutput(TlsClientProtocol &client, TlsServerProtocol &server, const bool fragmented)
        -> bool {
        auto progress = false;
        while (auto record = client.takeTransportOutput()) {
            progress = true;
            if (fragmented) {
                for (const auto byte : record->span()) {
                    server.feedTransport(ByteBlock({byte}).span());
                }
            } else {
                server.feedTransport(record->span());
            }
        }
        return progress;
    }

    static auto transferServerOutput(TlsServerProtocol &server, TlsClientProtocol &client, const bool fragmented)
        -> bool {
        auto progress = false;
        while (auto record = server.takeTransportOutput()) {
            progress = true;
            if (fragmented) {
                for (const auto byte : record->span()) {
                    client.feedTransport(ByteBlock({byte}).span());
                }
            } else {
                client.feedTransport(record->span());
            }
        }
        return progress;
    }

    void completeHandshake(
        TlsClientProtocol &client,
        TlsServerProtocol &server,
        const bool fragmented,
        const bool deterministicServerInputs = false) {
        for (auto iteration = 0U; iteration < 100U; ++iteration) {
            auto progress = transferClientOutput(client, server, fragmented);
            if (server.checkpoint() == TlsServerProtocolCheckpoint::ClientHello) {
                REQUIRE(server.serverName().has_value());
                REQUIRE_EQUAL(server.serverName()->toString(HostNameFormat::IdnaAscii), "localhost"_el);
                if (deterministicServerInputs) {
                    TlsServerProtocolTestAccess{server}.resume(ByteBlock{ByteLength{32U}, 0x66U}, privateKey(0x77U));
                } else {
                    server.resume();
                }
                progress = true;
            }
            progress = transferServerOutput(server, client, fragmented) || progress;
            while (client.hasCheckpoint()) {
                client.resume();
                progress = true;
            }
            if (server.checkpoint() == TlsServerProtocolCheckpoint::HandshakeCompleted) {
                server.resume();
                progress = true;
            }
            if (client.state() == TlsClientProtocolState::Established &&
                server.state() == TlsServerProtocolState::Established && !client.hasCheckpoint() &&
                !server.hasCheckpoint()) {
                return;
            }
            REQUIRE(progress);
        }
        REQUIRE(false);
    }

public:
    void testOptionsRejectInvalidIdentitySuitesAndAlpnBounds() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto serverIdentity = identity();
        REQUIRE_THROWS_AS(el::err::ParameterError, (TlsServerProtocolOptions{nullptr}));
        REQUIRE_THROWS_AS(
            el::err::ParameterError, (TlsServerProtocolOptions{serverIdentity, {}, std::vector<TlsCipherSuite>{}}));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            (TlsServerProtocolOptions{
                serverIdentity, {}, {TlsCipherSuite::Aes128GcmSha256, TlsCipherSuite::Aes128GcmSha256}}));
        REQUIRE_THROWS_AS(el::err::ParameterError, (TlsServerProtocolOptions{serverIdentity, {el::text::String{}}}));
        const auto repeatedProtocol = el::text::String{"h2"_el};
        REQUIRE_THROWS_AS(
            el::err::ParameterError, (TlsServerProtocolOptions{serverIdentity, {repeatedProtocol, repeatedProtocol}}));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            (TlsServerProtocolOptions{
                serverIdentity, {TlsAlpnProtocol::fromBytes(ByteBlock{ByteLength{256U}, el::mem::Byte{1U}}.span())}}));

        auto tooManyProtocols = std::vector<el::text::String>(65U, "x"_el);
        REQUIRE_THROWS_AS(
            el::err::ParameterError, (TlsServerProtocolOptions{serverIdentity, std::move(tooManyProtocols)}));
        auto oversizedProtocols = std::vector<el::text::String>{};
        for (auto index = uint8_t{0U}; index < 17U; ++index) {
            oversizedProtocols.emplace_back(
                TlsAlpnProtocol::fromBytes(ByteBlock{ByteLength{255U}, el::mem::Byte{index}}.span()));
        }
        REQUIRE_THROWS_AS(
            el::err::ParameterError, (TlsServerProtocolOptions{serverIdentity, std::move(oversizedProtocols)}));
        const auto malformedOne = TlsAlpnProtocol::fromBytes(ByteBlock({0xFFU}).span());
        const auto malformedTwo = TlsAlpnProtocol::fromBytes(ByteBlock({0xFEU}).span());
        REQUIRE_NOTHROW((TlsServerProtocolOptions{serverIdentity, {malformedOne, malformedTwo}}));

        const auto name = HostName::fromStringOrThrow("WWW.bücher.example"_el);
        auto duplicateNames = std::vector<TlsServerProtocolOptions::NamedIdentity>{
            {name, serverIdentity}, {HostName::fromStringOrThrow("www.XN--BCHER-KVA.example"_el), serverIdentity}};
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            (TlsServerProtocolOptions::withNamedIdentities(
                serverIdentity, std::move(duplicateNames), {}, {TlsCipherSuite::Aes128GcmSha256})));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            (TlsServerProtocolOptions::withNamedIdentities(
                serverIdentity,
                {{HostName::fromStringOrThrow("named.example"_el), nullptr}},
                {},
                {TlsCipherSuite::Aes128GcmSha256})));
    }

    void testExactCanonicalSniSelectionAndDefaultFallback() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto defaultIdentity = identity();
        const auto namedIdentity =
            identity("data/network/tls-interop/server-ecdsa.pem", "data/network/tls-interop/server-ecdsa-key.pem");
        const auto canonicalName = HostName::fromStringOrThrow("www.bücher.example"_el);
        const auto makeServer = [&]() -> TlsServerProtocol {
            return TlsServerProtocol{TlsServerProtocolOptions::withNamedIdentities(
                defaultIdentity, {{canonicalName, namedIdentity}}, {}, {TlsCipherSuite::Aes128GcmSha256})};
        };

        auto exactClient = TlsClientProtocol{clientOptionsForHost("WWW.XN--BCHER-KVA.EXAMPLE"_el)};
        auto exactServer = makeServer();
        exactClient.start();
        exactServer.start();
        REQUIRE(transferClientOutput(exactClient, exactServer, false));
        REQUIRE_EQUAL(exactServer.checkpoint(), TlsServerProtocolCheckpoint::ClientHello);
        REQUIRE(exactServer.selectedIdentityIndex().has_value());
        REQUIRE_EQUAL(*exactServer.selectedIdentityIndex(), 1U);
        REQUIRE_EQUAL(*exactServer.signatureScheme(), TlsSignatureScheme::EcdsaSecp256r1Sha256);
        exactServer.abort();

        auto fallbackClient = TlsClientProtocol{clientOptionsForHost("unknown.example"_el)};
        auto fallbackServer = makeServer();
        fallbackClient.start();
        fallbackServer.start();
        REQUIRE(transferClientOutput(fallbackClient, fallbackServer, false));
        REQUIRE_EQUAL(fallbackServer.checkpoint(), TlsServerProtocolCheckpoint::ClientHello);
        REQUIRE(fallbackServer.selectedIdentityIndex().has_value());
        REQUIRE_EQUAL(*fallbackServer.selectedIdentityIndex(), 0U);
        REQUIRE_EQUAL(*fallbackServer.signatureScheme(), TlsSignatureScheme::RsaPssRsaeSha256);
        fallbackServer.abort();
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAuthenticatedHandshakeApplicationAndFragmentation() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto http11 = el::text::String{"http/1.1"_el};
        const auto http2 = el::text::String{"h2"_el};
        auto client = TlsClientProtocol{clientOptions({http11, http2})};
        auto server =
            TlsServerProtocol{TlsServerProtocolOptions{identity(), {http2, http11}, {TlsCipherSuite::Aes128GcmSha256}}};
        auto access = TlsServerProtocolTestAccess{server};
        client.start();
        server.start();
        completeHandshake(client, server, true);

        REQUIRE_EQUAL(server.negotiatedAlpn(), http2);
        REQUIRE(server.cipherSuite().has_value());
        REQUIRE_EQUAL(*server.cipherSuite(), TlsCipherSuite::Aes128GcmSha256);
        REQUIRE(server.signatureScheme().has_value());

        const auto malformed = TlsAlpnProtocol::fromBytes(ByteBlock({0xFFU, 0xFEU}).span());
        auto rawClient = TlsClientProtocol{clientOptions({malformed})};
        auto rawServer =
            TlsServerProtocol{TlsServerProtocolOptions{identity(), {malformed}, {TlsCipherSuite::Aes128GcmSha256}}};
        rawClient.start();
        rawServer.start();
        completeHandshake(rawClient, rawServer, false);
        REQUIRE(TlsAlpnProtocol::equal(rawServer.negotiatedAlpn(), malformed));
        REQUIRE(TlsAlpnProtocol::equal(rawClient.negotiatedAlpn(), malformed));

        const auto request = ByteBlock({'p', 'i', 'n', 'g'});
        REQUIRE_EQUAL(client.sendApplication(request.span()), NetworkSendStatus::Accepted);
        REQUIRE(transferClientOutput(client, server, true));
        REQUIRE_EQUAL(*server.takeApplicationData(), request);

        const auto response = ByteBlock({'p', 'o', 'n', 'g'});
        REQUIRE_EQUAL(server.sendApplication(response.span()), NetworkSendStatus::Accepted);
        REQUIRE(transferServerOutput(server, client, true));
        REQUIRE_EQUAL(*client.takeApplicationData(), response);

        access.queueKeyUpdate(true);
        REQUIRE(transferServerOutput(server, client, false));
        REQUIRE(transferClientOutput(client, server, false));

        const auto afterUpdate = ByteBlock({'u', 'p', 'd', 'a', 't', 'e', 'd'});
        REQUIRE_EQUAL(server.sendApplication(afterUpdate.span()), NetworkSendStatus::Accepted);
        REQUIRE(transferServerOutput(server, client, false));
        REQUIRE_EQUAL(*client.takeApplicationData(), afterUpdate);

        client.close();
        REQUIRE(transferClientOutput(client, server, false));
        REQUIRE(server.peerCloseNotifyReceived());
        REQUIRE(transferServerOutput(server, client, false));
        REQUIRE_EQUAL(server.state(), TlsServerProtocolState::Closed);
        REQUIRE(access.securityStateIsEmpty());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testEveryCipherSuiteAndCheckpointAbortErasure() {
        const auto applicationScope = ApplicationTestScope<>{};
        for (const auto suite : TlsCipherSuite::all()) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    auto client = TlsClientProtocol{clientOptions()};
                    auto server = TlsServerProtocol{TlsServerProtocolOptions{identity(), {}, {suite}}};
                    client.start();
                    server.start();
                    completeHandshake(client, server, false, true);
                    REQUIRE_EQUAL(*server.cipherSuite(), suite);
                },
                [&]() -> std::string { return std::format("suite: 0x{:04x}", suite.toRawValue()); });
        }

        auto client = TlsClientProtocol{clientOptions()};
        auto server = TlsServerProtocol{TlsServerProtocolOptions{identity()}};
        auto access = TlsServerProtocolTestAccess{server};
        client.start();
        server.start();
        REQUIRE(transferClientOutput(client, server, false));
        REQUIRE_EQUAL(server.checkpoint(), TlsServerProtocolCheckpoint::ClientHello);
        server.abort();
        REQUIRE_EQUAL(server.state(), TlsServerProtocolState::Closed);
        REQUIRE(access.securityStateIsEmpty());
        REQUIRE_FALSE(server.hasTransportOutput());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRsaPssEcdsaP256AndEd25519Identities() {
        const auto applicationScope = ApplicationTestScope<>{};
        struct IdentityCase {
            const char *certificatePath;
            const char *keyPath;
            TlsSignatureScheme scheme;
        };
        static constexpr auto cCases = std::array{
            IdentityCase{
                "data/network/tls-interop/server.pem",
                "data/network/tls-interop/server-key.pem",
                TlsSignatureScheme::RsaPssRsaeSha256},
            IdentityCase{
                "data/network/tls-interop/server-ecdsa.pem",
                "data/network/tls-interop/server-ecdsa-key.pem",
                TlsSignatureScheme::EcdsaSecp256r1Sha256},
            IdentityCase{
                "data/network/tls-interop/server-ed25519.pem",
                "data/network/tls-interop/server-ed25519-key.pem",
                TlsSignatureScheme::Ed25519}};

        for (const auto &testCase : cCases) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    auto client = TlsClientProtocol{clientOptions({}, testCase.certificatePath)};
                    auto server = TlsServerProtocol{TlsServerProtocolOptions{
                        identity(testCase.certificatePath, testCase.keyPath), {}, {TlsCipherSuite::Aes128GcmSha256}}};
                    client.start();
                    server.start();
                    completeHandshake(client, server, false);
                    REQUIRE_EQUAL(*server.signatureScheme(), testCase.scheme);
                },
                [&]() -> std::string {
                    return std::format("signature scheme: 0x{:04x}", testCase.scheme.toRawValue());
                });
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testWrongFinishedPrematureApplicationAndKeyBoundaryCrossing() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto serverIdentity = identity();

        const auto prepareServer = [&](TlsClientProtocol &client, TlsServerProtocol &server) -> void {
            client.start();
            server.start();
            REQUIRE(transferClientOutput(client, server, false));
            REQUIRE_EQUAL(server.checkpoint(), TlsServerProtocolCheckpoint::ClientHello);
            server.resume();
            REQUIRE_EQUAL(server.state(), TlsServerProtocolState::Handshaking);
            REQUIRE_FALSE(server.hasCheckpoint());
        };
        const auto invalidFinished = []() -> ByteBlock {
            auto writer = TlsWireWriter{};
            writer.writeU8(20U);
            writer.writeU24(32U);
            writer.writeBytes(ByteBlock{ByteLength{32U}}.span());
            return writer.finish();
        };

        auto wrongFinishedClient = TlsClientProtocol{clientOptions()};
        auto wrongFinishedServer =
            TlsServerProtocol{TlsServerProtocolOptions{serverIdentity, {}, {TlsCipherSuite::Aes128GcmSha256}}};
        prepareServer(wrongFinishedClient, wrongFinishedServer);
        auto wrongFinishedAccess = TlsServerProtocolTestAccess{wrongFinishedServer};
        auto wrongFinishedSender = wrongFinishedAccess.clientHandshakeEncryptor();
        wrongFinishedServer.feedTransport(
            wrongFinishedSender.protect(TlsRecordContentType::Handshake, invalidFinished().span()).span());
        REQUIRE_EQUAL(wrongFinishedServer.state(), TlsServerProtocolState::Failed);
        REQUIRE_EQUAL(*wrongFinishedServer.failureAlert(), TlsAlertDescription::DecryptError);
        REQUIRE(wrongFinishedAccess.securityStateIsEmpty());

        auto prematureClient = TlsClientProtocol{clientOptions()};
        auto prematureServer =
            TlsServerProtocol{TlsServerProtocolOptions{serverIdentity, {}, {TlsCipherSuite::Aes128GcmSha256}}};
        prepareServer(prematureClient, prematureServer);
        auto prematureAccess = TlsServerProtocolTestAccess{prematureServer};
        auto prematureSender = prematureAccess.clientHandshakeEncryptor();
        prematureServer.feedTransport(
            prematureSender.protect(TlsRecordContentType::ApplicationData, ByteBlock({'n', 'o'}).span()).span());
        REQUIRE_EQUAL(prematureServer.state(), TlsServerProtocolState::Failed);
        REQUIRE_EQUAL(*prematureServer.failureAlert(), TlsAlertDescription::UnexpectedMessage);
        REQUIRE(prematureAccess.securityStateIsEmpty());

        auto crossedClient = TlsClientProtocol{clientOptions()};
        auto crossedServer =
            TlsServerProtocol{TlsServerProtocolOptions{serverIdentity, {}, {TlsCipherSuite::Aes128GcmSha256}}};
        prepareServer(crossedClient, crossedServer);
        auto crossedAccess = TlsServerProtocolTestAccess{crossedServer};
        auto crossedSender = crossedAccess.clientHandshakeEncryptor();
        auto crossedMessages = el::mem::ByteBlockEditor{};
        crossedMessages.append(invalidFinished());
        crossedMessages.append(ByteBlock({24U, 0U, 0U, 1U, 0U}));
        crossedServer.feedTransport(
            crossedSender.protect(TlsRecordContentType::Handshake, crossedMessages.span()).span());
        REQUIRE_EQUAL(crossedServer.state(), TlsServerProtocolState::Failed);
        REQUIRE_EQUAL(*crossedServer.failureAlert(), TlsAlertDescription::UnexpectedMessage);
        REQUIRE(crossedAccess.securityStateIsEmpty());
    }

    void testRejectedOffersBackPressureTimeoutAndExactlyOnceFailure() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto serverIdentity = identity();
        const auto http11 = el::text::String{"http/1.1"_el};
        const auto http2 = el::text::String{"h2"_el};

        auto noOverlapClient = TlsClientProtocol{clientOptions({http2})};
        auto noOverlapServer = TlsServerProtocol{TlsServerProtocolOptions{serverIdentity, {http11}}};
        noOverlapClient.start();
        noOverlapServer.start();
        REQUIRE(transferClientOutput(noOverlapClient, noOverlapServer, false));
        REQUIRE_EQUAL(noOverlapServer.state(), TlsServerProtocolState::Failed);
        REQUIRE_EQUAL(*noOverlapServer.failureAlert(), TlsAlertDescription::NoApplicationProtocol);

        auto malformedClient = TlsClientProtocol{clientOptions()};
        auto malformedServer = TlsServerProtocol{TlsServerProtocolOptions{serverIdentity}};
        malformedClient.start();
        malformedServer.start();
        auto clientHello = malformedClient.takeTransportOutput();
        REQUIRE(clientHello.has_value());
        auto malformed = el::mem::ByteBlockEditor{*clientHello};
        malformed.set(el::unit::ByteIndex{10U}, el::mem::Byte{0x02U});
        malformedServer.feedTransport(ByteBlock{malformed}.span());
        REQUIRE_EQUAL(malformedServer.state(), TlsServerProtocolState::Failed);
        REQUIRE_EQUAL(*malformedServer.failureAlert(), TlsAlertDescription::ProtocolVersion);

        auto zeroShareClient = TlsClientProtocol{clientOptions()};
        auto zeroShareClientAccess = TlsClientProtocolTestAccess{zeroShareClient};
        const auto clientPrivateKey = privateKey(0x44U);
        const auto clientPublicKey = clientPrivateKey.publicKey();
        zeroShareClientAccess.start(
            ByteBlock{ByteLength{32U}, 0x11U}, ByteBlock{ByteLength{32U}, 0x22U}, privateKey(0x44U));
        auto zeroShareHello = zeroShareClient.takeTransportOutput();
        REQUIRE(zeroShareHello.has_value());
        const auto publicKeyOffset = zeroShareHello->find(clientPublicKey.span());
        REQUIRE_FALSE(publicKeyOffset.isNoIndex());
        auto zeroShareEditor = el::mem::ByteBlockEditor{*zeroShareHello};
        zeroShareEditor.replace(
            el::unit::ByteRange{publicKeyOffset, ByteLength{32U}}, ByteBlock{ByteLength{32U}}.span());
        auto zeroShareServer =
            TlsServerProtocol{TlsServerProtocolOptions{serverIdentity, {}, {TlsCipherSuite::Aes128GcmSha256}}};
        zeroShareServer.start();
        zeroShareServer.feedTransport(zeroShareEditor.span());
        REQUIRE_EQUAL(zeroShareServer.checkpoint(), TlsServerProtocolCheckpoint::ClientHello);
        zeroShareServer.resume();
        REQUIRE_EQUAL(zeroShareServer.state(), TlsServerProtocolState::Failed);
        REQUIRE_EQUAL(*zeroShareServer.failureAlert(), TlsAlertDescription::IllegalParameter);

        auto boundedClient = TlsClientProtocol{clientOptions()};
        auto boundedServer = TlsServerProtocol{TlsServerProtocolOptions{
            serverIdentity,
            {},
            {TlsCipherSuite::Aes128GcmSha256},
            SocketBufferLimits{ByteLength{512U}, ByteLength{1024U * 1024U}}}};
        auto boundedAccess = TlsServerProtocolTestAccess{boundedServer};
        boundedClient.start();
        boundedServer.start();
        REQUIRE(transferClientOutput(boundedClient, boundedServer, false));
        REQUIRE_EQUAL(boundedServer.checkpoint(), TlsServerProtocolCheckpoint::ClientHello);
        boundedServer.resume();
        REQUIRE_EQUAL(boundedServer.state(), TlsServerProtocolState::Failed);
        REQUIRE_EQUAL(*boundedServer.failureAlert(), TlsAlertDescription::InternalError);
        REQUIRE(boundedAccess.securityStateIsEmpty());
        while (const auto output = boundedServer.takeTransportOutput()) {
            REQUIRE_NOT_EQUAL(output->getOrThrow(el::unit::ByteIndex::zero()), el::mem::Byte{22U});
        }

        auto timedOut = TlsServerProtocol{TlsServerProtocolOptions{serverIdentity}};
        auto timedOutAccess = TlsServerProtocolTestAccess{timedOut};
        timedOut.start();
        timedOut.timeout();
        REQUIRE_EQUAL(timedOut.state(), TlsServerProtocolState::Failed);
        REQUIRE(timedOutAccess.securityStateIsEmpty());
        const auto firstDiagnostic = timedOut.failureDiagnostic();
        timedOut.timeout();
        timedOut.transportClosed();
        timedOut.abort();
        REQUIRE_EQUAL(timedOut.state(), TlsServerProtocolState::Failed);
        REQUIRE_EQUAL(timedOut.failureDiagnostic(), firstDiagnostic);

        auto truncated = TlsServerProtocol{TlsServerProtocolOptions{serverIdentity}};
        truncated.start();
        truncated.transportClosed();
        REQUIRE_EQUAL(truncated.state(), TlsServerProtocolState::Failed);
        REQUIRE(truncated.failureWasTruncation());
    }
};
