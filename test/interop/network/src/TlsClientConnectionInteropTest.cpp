// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "InteropEnvironment.hpp"

#include "core/ApplicationTestScope.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/CryptologyConfiguration.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/HostName.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Network.hpp>
#include <erbsland/NetworkErrorContext.hpp>
#include <erbsland/SigningPrivateKey.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/TlsAlertDescription.hpp>
#include <erbsland/TlsCipherSuite.hpp>
#include <erbsland/TlsClientConnection.hpp>
#include <erbsland/TlsConfiguration.hpp>
#include <erbsland/TlsIdentity.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/X509CertificateBundle.hpp>
#include <erbsland/X509TlsCertificatePolicy.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

using namespace el::cryptology;
using namespace el::event;
using namespace el::network;
using namespace el::text::literals;
namespace mem = el::mem;
namespace unit = el::unit;

TESTED_TARGETS(TlsClientConnection TlsClientProtocol)
class TlsClientConnectionInteropTest final : public el::UnitTest {
private:
    struct Result final {
        bool peerAuthenticated{};
        bool handshakeCompleted{};
        bool closed{};
        bool final{};
        bool sendClosed{};
        unit::ByteCount sent{};
        mem::ByteBlockEditor received;
        std::optional<TlsCipherSuite> cipherSuite;
        el::text::String alpn;
        std::optional<NetworkErrorContext> error;
    };

    static constexpr auto cChunkLength = unit::ByteCount{16U * 1024U};

private:
    [[nodiscard]] static auto alpn() -> el::text::String { return "erbsland-test"_el; }

    static void installTlsConfiguration(const bool clientIdentity = false, const bool trustServer = true) {
        const auto ca = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/ca.pem")});
        auto anchors = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/client-ca.pem")});
        if (trustServer) {
            anchors = ca;
        }
        auto configuration = TlsConfiguration{TlsClientConfiguration{TlsTrustConfiguration{anchors}}};
        if (clientIdentity) {
            configuration.setClient(
                el::cryptology::TlsClientConfiguration{
                    configuration.client()->trust(),
                    std::make_shared<const el::cryptology::TlsIdentity>(TlsIdentity{
                        X509CertificateBundle::fromPemOrThrow(
                            el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/client.pem")}),
                        SigningPrivateKey::fromPemOrThrow(
                            el::text::String{
                                el::unittest::fh::readDataText("data/network/tls-interop/client-key.pem")})})});
        }
        el::core::application().cryptologyConfiguration().setTlsConfiguration(
            "tls/interop"_el, std::move(configuration));
    }

    static void pump(const TlsClientConnectionPtr &connection, Result &result, const unit::ByteCount payloadLength) {
        while (result.sent < payloadLength) {
            const auto length = std::min(cChunkLength, payloadLength - result.sent);
            const auto status = connection->send(mem::ByteBlock{length, 0x5aU});
            if (status.wouldBlock()) {
                return;
            }
            if (status.isClosed()) {
                result.sendClosed = true;
                return;
            }
            result.sent += length;
        }
    }

    void runUntilFinal(const EventLoopPtr &loop, Result &result) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{20};
        while (!result.final && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(el::time::Milliseconds{20}));
        }
        REQUIRE(result.final);
    }

    auto runScenario(
        const std::string &scenario,
        const std::string &cipher,
        const unit::ByteCount payloadLength,
        const bool closeAfterEcho,
        const std::vector<KeyAgreementAlgorithm> &groups = {},
        const bool clientIdentity = false) -> Result {
        auto peer = InteropEnvironment::instance().startScenario(scenario, cipher, payloadLength.toRawValue());
        const auto applicationScope = ApplicationTestScope<>{};
        installTlsConfiguration(clientIdentity);
        const auto loop = EventLoop::create();
        auto connection = TlsClientConnectionPtr{};
        auto result = Result{};
        auto closeRequested = false;
        loop->invoke([&]() -> void {
            connection = loop->get<Network>().createTlsClientConnection();
            connection->events()
                .onPeerAuthenticated([&]() -> void { result.peerAuthenticated = true; })
                .onHandshakeCompleted([&]() -> void {
                    REQUIRE(result.peerAuthenticated);
                    result.handshakeCompleted = true;
                    result.cipherSuite = connection->cipherSuite();
                    result.alpn = connection->negotiatedAlpn();
                    pump(connection, result, payloadLength);
                })
                .onWritable([&]() -> void { pump(connection, result, payloadLength); })
                .onData([&](mem::ByteBlock data) -> void {
                    result.received.append(data);
                    if (closeAfterEcho && !closeRequested && result.received.length() >= payloadLength) {
                        closeRequested = true;
                        connection->close();
                    }
                })
                .onClosed([&](const ConnectionCloseContext &) -> void { result.closed = true; })
                .onError([&](const NetworkErrorContext &context) -> void { result.error = context; })
                .onFinal([&]() -> void { result.final = true; });
            auto options = TlsClientConnectOptions{};
            if (!groups.empty()) {
                options.setSupportedGroups(groups);
            }
            options.setConfigurationLabel("tls/interop"_el)
                .setAlpnProtocols(el::StringList{alpn()})
                .setBufferLimits(
                    SocketBufferLimits{unit::ByteCount{8U * 1024U * 1024U}, unit::ByteCount{8U * 1024U * 1024U}})
                .setHandshakeTimeout(el::time::TimeDelta::seconds(10))
                .setIdleTimeout(el::time::TimeDelta::seconds(10))
                .setCloseTimeout(el::time::TimeDelta::seconds(10));
            connection->connect(
                HostEndpoint{HostName::fromStringOrThrow("localhost"_el), Port{peer.port()}}, std::move(options));
        });
        runUntilFinal(loop, result);
        const auto peerBytes = peer.finish();
        REQUIRE_EQUAL(peerBytes, payloadLength.toRawValue());
        return result;
    }

    void testSuccessfulCipher(const std::string &cipher, const TlsCipherSuite expectedSuite) {
        const auto result = runScenario("tls_echo", cipher, unit::ByteCount{4U}, true);
        REQUIRE(result.handshakeCompleted);
        REQUIRE(result.closed);
        REQUIRE_FALSE(result.sendClosed);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE_EQUAL(result.sent, unit::ByteCount{4U});
        REQUIRE_EQUAL(mem::ByteBlock{result.received}, mem::ByteBlock({0x5aU, 0x5aU, 0x5aU, 0x5aU}));
        REQUIRE_EQUAL(result.cipherSuite, std::optional<TlsCipherSuite>{expectedSuite});
        REQUIRE_EQUAL(result.alpn, alpn());
    }

    auto runFailure(const std::string &scenario, const bool clientIdentity = false, const bool trustServer = true)
        -> Result {
        auto peer = InteropEnvironment::instance().startScenario(scenario, "", 0U);
        const auto applicationScope = ApplicationTestScope<>{};
        installTlsConfiguration(clientIdentity, trustServer);
        const auto loop = EventLoop::create();
        auto result = Result{};
        auto connection = TlsClientConnectionPtr{};
        loop->invoke([&]() -> void {
            connection = loop->get<Network>().createTlsClientConnection();
            connection->events()
                .onPeerAuthenticated([&]() -> void { result.peerAuthenticated = true; })
                .onHandshakeCompleted([&]() -> void { result.handshakeCompleted = true; })
                .onError([&](const NetworkErrorContext &context) -> void { result.error = context; })
                .onFinal([&]() -> void { result.final = true; });
            auto options = TlsClientConnectOptions{};
            options.setConfigurationLabel("tls/interop"_el).setHandshakeTimeout(el::time::TimeDelta::seconds(10));
            connection->connect(
                HostEndpoint{HostName::fromStringOrThrow("localhost"_el), Port{peer.port()}}, std::move(options));
        });
        runUntilFinal(loop, result);
        REQUIRE_EQUAL(peer.finish(), std::uint64_t{0U});
        return result;
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testInitialHandshakeClientAuthentication() {
        const auto result =
            runScenario("tls_mtls_required", "TLS_AES_128_GCM_SHA256", unit::ByteCount{4U}, true, {}, true);
        REQUIRE(result.peerAuthenticated);
        REQUIRE(result.handshakeCompleted);
        REQUIRE(result.closed);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE_EQUAL(result.received.length(), unit::ByteCount{4U});
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNoConfiguredClientIdentitySendsEmptyCertificate() {
        const auto result = runScenario("tls_mtls_empty", "TLS_AES_128_GCM_SHA256", unit::ByteCount{4U}, true);
        REQUIRE(result.handshakeCompleted);
        REQUIRE_FALSE(result.error.has_value());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMismatchedIssuerSendsEmptyCertificate() {
        const auto result =
            runScenario("tls_mtls_issuer_mismatch", "TLS_AES_128_GCM_SHA256", unit::ByteCount{4U}, true, {}, true);
        REQUIRE(result.handshakeCompleted);
        REQUIRE_FALSE(result.error.has_value());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testClientIdentityIsNotDisclosedToUntrustedServer() {
        const auto result = runFailure("tls_mtls_untrusted_server", true, false);
        REQUIRE_FALSE(result.peerAuthenticated);
        REQUIRE_FALSE(result.handshakeCompleted);
        REQUIRE(result.error.has_value());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun LiveNetwork)
    void testLiveSystemTrust() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto loop = EventLoop::create();
        auto connection = TlsClientConnectionPtr{};
        auto result = Result{};
        loop->invoke([&]() -> void {
            connection = loop->get<Network>().createTlsClientConnection();
            connection->events()
                .onPeerAuthenticated([&]() -> void { result.peerAuthenticated = true; })
                .onHandshakeCompleted([&]() -> void {
                    result.handshakeCompleted = true;
                    connection->close();
                })
                .onError([&](const NetworkErrorContext &context) -> void { result.error = context; })
                .onFinal([&]() -> void { result.final = true; });
            auto options = TlsClientConnectOptions{};
            options.setHandshakeTimeout(el::time::TimeDelta::seconds(15))
                .setCloseTimeout(el::time::TimeDelta::seconds(3));
            connection->connect(
                HostEndpoint{HostName::fromStringOrThrow("www.cloudflare.com"_el), Port{443}}, std::move(options));
        });
        runUntilFinal(loop, result);
        REQUIRE(result.peerAuthenticated);
        REQUIRE(result.handshakeCompleted);
    }

    void testAes128GcmSha256() { testSuccessfulCipher("TLS_AES_128_GCM_SHA256", TlsCipherSuite::Aes128GcmSha256); }

    void testAes256GcmSha384() { testSuccessfulCipher("TLS_AES_256_GCM_SHA384", TlsCipherSuite::Aes256GcmSha384); }

    void testP256RetryAgainstRustls() {
        const auto result = runScenario("tls_echo_p256", "TLS_AES_256_GCM_SHA384", unit::ByteCount{4U}, true);
        REQUIRE(result.handshakeCompleted);
        REQUIRE(result.closed);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE_EQUAL(result.received.length(), unit::ByteCount{4U});
    }

    void testP256InitialShareAgainstRustls() {
        const auto result = runScenario(
            "tls_echo_p256",
            "TLS_AES_128_GCM_SHA256",
            unit::ByteCount{4U},
            true,
            {KeyAgreementAlgorithm::P256, KeyAgreementAlgorithm::X25519});
        REQUIRE(result.handshakeCompleted);
        REQUIRE(result.closed);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE_EQUAL(result.received.length(), unit::ByteCount{4U});
    }

    void testChaCha20Poly1305Sha256() {
        testSuccessfulCipher("TLS_CHACHA20_POLY1305_SHA256", TlsCipherSuite::ChaCha20Poly1305Sha256);
    }

    void testTls12OnlyPeer() {
        const auto result = runFailure("tls12_only");
        REQUIRE_FALSE(result.handshakeCompleted);
        REQUIRE(result.error.has_value());
        REQUIRE_EQUAL(result.error->reason(), NetworkErrorReason::TlsPeerAlert);
        REQUIRE_EQUAL(result.error->phase(), NetworkErrorPhase::Handshaking);
        REQUIRE_EQUAL(
            result.error->tlsAlert(), std::optional<TlsAlertDescription>{TlsAlertDescription::ProtocolVersion});
    }

    void testUnsupportedServerCipher() {
        const auto result = runFailure("unsupported_server_cipher");
        REQUIRE_FALSE(result.handshakeCompleted);
        REQUIRE(result.error.has_value());
        REQUIRE_EQUAL(result.error->reason(), NetworkErrorReason::TlsProtocolFailure);
        REQUIRE_EQUAL(result.error->phase(), NetworkErrorPhase::Handshaking);
        REQUIRE_EQUAL(
            result.error->tlsAlert(), std::optional<TlsAlertDescription>{TlsAlertDescription::IllegalParameter});
    }

    void testInterruptedHandshake() {
        const auto result = runFailure("disconnect_during_handshake");
        REQUIRE_FALSE(result.handshakeCompleted);
        REQUIRE(result.error.has_value());
        REQUIRE_EQUAL(result.error->reason(), NetworkErrorReason::TlsTruncation);
        REQUIRE_EQUAL(result.error->phase(), NetworkErrorPhase::Handshaking);
    }

    void testTruncatedActiveConnection() {
        const auto result = runScenario("truncate_after_data", "TLS_AES_128_GCM_SHA256", unit::ByteCount{4U}, false);
        REQUIRE(result.handshakeCompleted);
        REQUIRE_FALSE(result.closed);
        REQUIRE(result.error.has_value());
        REQUIRE_EQUAL(result.error->reason(), NetworkErrorReason::TlsTruncation);
        REQUIRE_EQUAL(result.error->phase(), NetworkErrorPhase::Active);
        REQUIRE_EQUAL(result.received.length(), unit::ByteCount{4U});
    }

    void testFourMegabyteDuplexStream() {
        constexpr auto cPayloadLength = unit::ByteCount{4U * 1024U * 1024U};
        const auto result = runScenario("tls_echo", "TLS_AES_128_GCM_SHA256", cPayloadLength, true);
        REQUIRE(result.handshakeCompleted);
        REQUIRE(result.closed);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE_EQUAL(result.sent, cPayloadLength);
        REQUIRE_EQUAL(result.received.length(), cPayloadLength);
        const auto data = mem::ByteBlock{result.received};
        for (const auto value : data.span()) {
            REQUIRE_EQUAL(value, std::uint8_t{0x5aU});
        }
    }
};
