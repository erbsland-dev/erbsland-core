// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/KeyAgreementAlgorithm.hpp>
#include <erbsland/KeyAgreementPrivateKey.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocol.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolOptions.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocolTestAccess.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocol.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocolTestAccess.hpp>
#include <erbsland/network/impl/tls/TlsAlpnProtocol.hpp>
#include <erbsland/network/impl/tls/TlsWireWriter.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/SigningPrivateKey.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/text/impl/UnsafeU8StringBuffer.hpp>
#include <erbsland/Timestamp.hpp>
#include <erbsland/TlsIdentity.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/X509CertificateBundle.hpp>
#include <erbsland/X509TlsCertificatePolicy.hpp>

#include <array>
#include <format>
#include <memory>
#include <utility>
#include <vector>

using namespace el::cryptology;
using namespace el::network;
using namespace el::text::literals;
using el::mem::ByteBlock;
using el::unit::ByteCount;

TESTED_TARGETS(TlsServerProtocol TlsServerProtocolOptions TlsServerProtocolState TlsServerProtocolCheckpoint)
class TlsServerProtocolTest final : public el::UnitTest {
    using TlsClientProtocol = el::network::impl::TlsClientProtocol;
    using TlsClientProtocolOptions = el::network::impl::TlsClientProtocolOptions;
    using TlsClientProtocolState = el::network::impl::TlsClientProtocolState;
    using TlsClientProtocolTestAccess = el::network::impl::TlsClientProtocolTestAccess;
    using TlsServerProtocol = el::network::impl::TlsServerProtocol;
    using TlsServerProtocolCheckpoint = el::network::impl::TlsServerProtocolCheckpoint;
    using TlsServerProtocolOptions = el::network::impl::TlsServerProtocolOptions;
    using TlsServerProtocolState = el::network::impl::TlsServerProtocolState;
    using TlsServerProtocolTestAccess = el::network::impl::TlsServerProtocolTestAccess;
    using TlsWireWriter = el::network::impl::TlsWireWriter;

private:
    [[nodiscard]] static auto readText(const char *path) -> el::text::String {
        return el::text::String{el::unittest::fh::readDataText(path)};
    }

    [[nodiscard]] static auto identity(
        const char *certificatePath = "data/network/tls-interop/server.pem",
        const char *keyPath = "data/network/tls-interop/server-key.pem") -> TlsIdentityConstPtr {
        return std::make_shared<const TlsIdentity>(
            X509CertificateBundle::fromPemOrThrow(readText(certificatePath)),
            SigningPrivateKey::fromPemOrThrow(readText(keyPath)));
    }

    [[nodiscard]] static auto clientOptions(
        el::StringList alpn = {}, const char *trustPath = "data/network/tls-interop/ca.pem")
        -> TlsClientProtocolOptions {
        return clientOptionsForHost("localhost"_el, std::move(alpn), trustPath);
    }

    [[nodiscard]] static auto clientOptionsForHost(
        const el::text::String &host,
        el::StringList alpn = {},
        const char *trustPath = "data/network/tls-interop/ca.pem") -> TlsClientProtocolOptions {
        return TlsClientProtocolOptions{
            Host::fromStringOrThrow(host),
            X509TlsCertificatePolicy{X509CertificateBundle::fromPemOrThrow(readText(trustPath))},
            el::time::Timestamp::now(),
            std::move(alpn)};
    }

    [[nodiscard]] static auto privateKey(const uint8_t value) -> KeyAgreementPrivateKey {
        return KeyAgreementPrivateKey::fromBytes(
            KeyAgreementAlgorithm::X25519, ByteBlock{ByteCount{32U}, value}.span());
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
                    TlsServerProtocolTestAccess{server}.resume(ByteBlock{ByteCount{32U}, 0x66U}, privateKey(0x77U));
                } else {
                    server.resume();
                }
                progress = true;
            }
            if (server.pendingSignature().has_value()) {
                const auto request = *server.pendingSignature();
                server.resume();
                REQUIRE(server.pendingSignature().has_value());
                server.completeSignature(request.execute());
                progress = true;
            }
            progress = transferServerOutput(server, client, fragmented) || progress;
            while (client.hasCheckpoint()) {
                if (client.pendingCertificateValidation().has_value()) {
                    const auto request = *client.pendingCertificateValidation();
                    client.completeCertificateValidation(request.execute());
                } else if (client.pendingSignature().has_value()) {
                    const auto request = *client.pendingSignature();
                    client.completeSignature(request.execute());
                } else {
                    client.resume();
                }
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
            REQUIRE_EQUAL(el::text::StringConverter{client.failureDiagnostic()}.toStdString(), std::string{});
            REQUIRE_EQUAL(el::text::StringConverter{server.failureDiagnostic()}.toStdString(), std::string{});
            REQUIRE(progress);
        }
        REQUIRE(false);
    }

public:
    void testServerAcceptsSuppliedShareBeforePreferredRetry() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto client = TlsClientProtocol{clientOptions()};
        auto server = TlsServerProtocol{TlsServerProtocolOptions{
            identity(),
            {},
            {TlsCipherSuite::Aes128GcmSha256},
            {},
            {KeyAgreementAlgorithm::P256, KeyAgreementAlgorithm::X25519}}};
        server.start();
        client.start();
        REQUIRE(transferClientOutput(client, server, false));
        REQUIRE_EQUAL(server.checkpoint(), TlsServerProtocolCheckpoint::ClientHello);
        REQUIRE_FALSE(server.takeTransportOutput().has_value());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testServerRejectsChangedSecondHelloRandom() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto client = TlsClientProtocol{clientOptions()};
        auto server = TlsServerProtocol{TlsServerProtocolOptions{
            identity(), {}, {TlsCipherSuite::Aes128GcmSha256}, {}, {KeyAgreementAlgorithm::P256}}};
        server.start();
        client.start();
        REQUIRE(transferClientOutput(client, server, false));
        REQUIRE(transferServerOutput(server, client, true));
        const auto ccs = client.takeTransportOutput();
        REQUIRE(ccs.has_value());
        server.feedTransport(ccs->span());
        const auto secondHello = client.takeTransportOutput();
        REQUIRE(secondHello.has_value());
        auto changed = el::mem::ByteBlockEditor{*secondHello};
        changed.set(el::unit::ByteIndex{11U}, changed.getOrThrow(el::unit::ByteIndex{11U}) ^ el::mem::Byte{1U});
        server.feedTransport(changed.span());
        REQUIRE_EQUAL(server.state(), TlsServerProtocolState::Failed);
        REQUIRE_EQUAL(*server.failureAlert(), TlsAlertDescription::IllegalParameter);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testP256HelloRetryRequestRoundTrip() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto client = TlsClientProtocol{clientOptions()};
        auto server = TlsServerProtocol{TlsServerProtocolOptions{
            identity(), {}, {TlsCipherSuite::Aes128GcmSha256}, {}, {KeyAgreementAlgorithm::P256}}};
        server.start();
        client.start();
        WITH_CONTEXT(completeHandshake(client, server, true));
        REQUIRE(client.state() == TlsClientProtocolState::Established);
        REQUIRE(server.state() == TlsServerProtocolState::Established);
    }

    void testOptionsRejectInvalidIdentitySuitesAndAlpnBounds() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto serverIdentity = identity();
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            (TlsServerProtocolOptions{el::network::impl::TlsServerProtocolConfiguration{nullptr}}));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, (TlsServerProtocolOptions{serverIdentity, {}, std::vector<TlsCipherSuite>{}}));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            (TlsServerProtocolOptions{
                serverIdentity, {}, {TlsCipherSuite::Aes128GcmSha256, TlsCipherSuite::Aes128GcmSha256}}));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, (TlsServerProtocolOptions{serverIdentity, el::StringList{el::text::String{}}}));
        const auto repeatedProtocol = el::text::String{"h2"_el};
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            (TlsServerProtocolOptions{serverIdentity, el::StringList{repeatedProtocol, repeatedProtocol}}));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            (TlsServerProtocolOptions{
                serverIdentity,
                el::StringList{
                    el::text::impl::UnsafeU8StringBuffer{ByteBlock{ByteCount{256U}, el::mem::Byte{1U}}.span()}
                        .takeString()}}));

        auto tooManyProtocols = el::StringList{};
        tooManyProtocols.resize(el::ItemCount{65U}, "x"_el);
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, (TlsServerProtocolOptions{serverIdentity, std::move(tooManyProtocols)}));
        auto oversizedProtocols = el::StringList{};
        for (auto index = uint8_t{0U}; index < 17U; ++index) {
            oversizedProtocols.append(
                el::text::impl::UnsafeU8StringBuffer{ByteBlock{ByteCount{255U}, el::mem::Byte{index}}.span()}
                    .takeString());
        }
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure, (TlsServerProtocolOptions{serverIdentity, std::move(oversizedProtocols)}));
        const auto malformedOne = el::text::impl::UnsafeU8StringBuffer{ByteBlock({0xFFU}).span()}.takeString();
        const auto malformedTwo = el::text::impl::UnsafeU8StringBuffer{ByteBlock({0xFEU}).span()}.takeString();
        REQUIRE_NOTHROW((TlsServerProtocolOptions{serverIdentity, el::StringList{malformedOne, malformedTwo}}));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            (TlsServerProtocolOptions{serverIdentity, el::StringList{malformedOne, malformedOne}}));

        const auto name = HostName::fromStringOrThrow("WWW.bücher.example"_el);
        auto duplicateNames = std::vector<TlsServerProtocolOptions::NamedConfiguration>{
            {name, serverIdentity}, {HostName::fromStringOrThrow("www.XN--BCHER-KVA.example"_el), serverIdentity}};
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            (TlsServerProtocolOptions::withNamedConfigurations(
                serverIdentity, std::move(duplicateNames), {}, {TlsCipherSuite::Aes128GcmSha256})));
        REQUIRE_THROWS_AS(
            el::err::ParameterFailure,
            (TlsServerProtocolOptions::withNamedConfigurations(
                serverIdentity,
                {{HostName::fromStringOrThrow("named.example"_el),
                    el::network::impl::TlsServerProtocolConfiguration{nullptr}}},
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
            return TlsServerProtocol{TlsServerProtocolOptions::withNamedConfigurations(
                defaultIdentity, {{canonicalName, namedIdentity}}, {}, {TlsCipherSuite::Aes128GcmSha256})};
        };

        auto exactClient = TlsClientProtocol{clientOptionsForHost("WWW.XN--BCHER-KVA.EXAMPLE"_el)};
        auto exactServer = makeServer();
        exactClient.start();
        exactServer.start();
        REQUIRE(transferClientOutput(exactClient, exactServer, false));
        REQUIRE_EQUAL(exactServer.checkpoint(), TlsServerProtocolCheckpoint::ClientHello);
        REQUIRE(exactServer.selectedConfigurationIndex().has_value());
        REQUIRE_EQUAL(*exactServer.selectedConfigurationIndex(), 1U);
        REQUIRE_EQUAL(*exactServer.signatureScheme(), TlsSignatureScheme::EcdsaSecp256r1Sha256);
        exactServer.abort();

        auto fallbackClient = TlsClientProtocol{clientOptionsForHost("unknown.example"_el)};
        auto fallbackServer = makeServer();
        fallbackClient.start();
        fallbackServer.start();
        REQUIRE(transferClientOutput(fallbackClient, fallbackServer, false));
        REQUIRE_EQUAL(fallbackServer.checkpoint(), TlsServerProtocolCheckpoint::ClientHello);
        REQUIRE(fallbackServer.selectedConfigurationIndex().has_value());
        REQUIRE_EQUAL(*fallbackServer.selectedConfigurationIndex(), 0U);
        REQUIRE_EQUAL(*fallbackServer.signatureScheme(), TlsSignatureScheme::RsaPssRsaeSha256);
        fallbackServer.abort();
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAuthenticatedHandshakeApplicationAndFragmentation() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto http11 = el::text::String{"http/1.1"_el};
        const auto http2 = el::text::String{"h2"_el};
        auto client = TlsClientProtocol{clientOptions(el::StringList{http11, http2})};
        auto server = TlsServerProtocol{
            TlsServerProtocolOptions{identity(), el::StringList{http2, http11}, {TlsCipherSuite::Aes128GcmSha256}}};
        auto access = TlsServerProtocolTestAccess{server};
        client.start();
        server.start();
        completeHandshake(client, server, true);

        REQUIRE_EQUAL(server.negotiatedAlpn(), http2);
        REQUIRE(server.cipherSuite().has_value());
        REQUIRE_EQUAL(*server.cipherSuite(), TlsCipherSuite::Aes128GcmSha256);
        REQUIRE(server.signatureScheme().has_value());

        const auto malformed = el::text::impl::UnsafeU8StringBuffer{ByteBlock({0xFFU, 0xFEU}).span()}.takeString();
        auto rawClient = TlsClientProtocol{clientOptions(el::StringList{malformed})};
        auto rawServer = TlsServerProtocol{
            TlsServerProtocolOptions{identity(), el::StringList{malformed}, {TlsCipherSuite::Aes128GcmSha256}}};
        rawClient.start();
        rawServer.start();
        completeHandshake(rawClient, rawServer, false);
        REQUIRE(erbsland::network::impl::tls_alpn_protocol::equal(rawServer.negotiatedAlpn(), malformed));
        REQUIRE(erbsland::network::impl::tls_alpn_protocol::equal(rawClient.negotiatedAlpn(), malformed));

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
                    auto client = TlsClientProtocol{clientOptions(el::StringList{}, testCase.certificatePath)};
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
            REQUIRE(server.pendingSignature().has_value());
            const auto request = *server.pendingSignature();
            server.completeSignature(request.execute());
            REQUIRE_EQUAL(server.state(), TlsServerProtocolState::Handshaking);
            REQUIRE_FALSE(server.hasCheckpoint());
        };
        const auto invalidFinished = []() -> ByteBlock {
            auto writer = TlsWireWriter{};
            writer.writeU8(20U);
            writer.writeU24(32U);
            writer.writeBytes(ByteBlock{ByteCount{32U}}.span());
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

        auto noOverlapClient = TlsClientProtocol{clientOptions(el::StringList{http2})};
        auto noOverlapServer = TlsServerProtocol{TlsServerProtocolOptions{serverIdentity, el::StringList{http11}}};
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
            ByteBlock{ByteCount{32U}, 0x11U}, ByteBlock{ByteCount{32U}, 0x22U}, privateKey(0x44U));
        auto zeroShareHello = zeroShareClient.takeTransportOutput();
        REQUIRE(zeroShareHello.has_value());
        const auto publicKeyOffset = zeroShareHello->find(clientPublicKey.span());
        REQUIRE_FALSE(publicKeyOffset.isNoIndex());
        auto zeroShareEditor = el::mem::ByteBlockEditor{*zeroShareHello};
        zeroShareEditor.replace(el::unit::ByteRange{publicKeyOffset, ByteCount{32U}}, ByteBlock{ByteCount{32U}}.span());
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
            SocketBufferLimits{ByteCount{512U}, ByteCount{1024U * 1024U}}}};
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
