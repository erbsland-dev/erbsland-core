// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "InteropEnvironment.hpp"

#include "core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/tls/TlsCipherSuite.hpp>
#include <erbsland/cryptology/tls/TlsConfiguration.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/cryptology/x509/X509ServerCertificatePolicy.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/network/HostName.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/source/NetworkErrorContext.hpp>
#include <erbsland/network/tls/TlsAlertDescription.hpp>
#include <erbsland/network/tls/TlsClientConnection.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

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
        bool handshakeCompleted{};
        bool closed{};
        bool final{};
        bool sendClosed{};
        std::uint64_t sent{};
        mem::ByteBlockEditor received;
        std::optional<TlsCipherSuite> cipherSuite;
        el::text::String alpn;
        std::optional<NetworkErrorContext> error;
    };

    static constexpr auto cChunkLength = std::uint64_t{16U * 1024U};

private:
    [[nodiscard]] static auto alpn() -> el::text::String {
        return "erbsland-test"_el;
    }

    static void installTlsConfiguration() {
        const auto ca = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/ca.pem")});
        el::core::application().cryptologyConfiguration().setTlsConfiguration(
            "tls/client/interop"_el, TlsConfiguration{X509ServerCertificatePolicy{ca}});
    }

    static void pump(const TlsClientConnectionPtr &connection, Result &result, const std::uint64_t payloadLength) {
        while (result.sent < payloadLength) {
            const auto length = std::min(cChunkLength, payloadLength - result.sent);
            const auto status = connection->send(mem::ByteBlock{unit::ByteLength{length}, 0x5aU});
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
        const std::uint64_t payloadLength,
        const bool closeAfterEcho) -> Result {
        auto peer = InteropEnvironment::instance().startScenario(scenario, cipher, payloadLength);
        const auto applicationScope = ApplicationTestScope<>{};
        installTlsConfiguration();
        const auto loop = EventLoop::create();
        auto connection = TlsClientConnectionPtr{};
        auto result = Result{};
        auto closeRequested = false;
        loop->invoke([&]() -> void {
            connection = loop->get<Network>().createTlsClientConnection();
            connection->events()
                .onHandshakeCompleted([&]() -> void {
                    result.handshakeCompleted = true;
                    result.cipherSuite = connection->cipherSuite();
                    result.alpn = connection->negotiatedAlpn();
                    pump(connection, result, payloadLength);
                })
                .onWritable([&]() -> void { pump(connection, result, payloadLength); })
                .onData([&](mem::ByteBlock data) -> void {
                    result.received.append(data);
                    if (closeAfterEcho && !closeRequested &&
                        result.received.length() >= unit::ByteLength{payloadLength}) {
                        closeRequested = true;
                        connection->close();
                    }
                })
                .onClosed([&](const ConnectionCloseContext &) -> void { result.closed = true; })
                .onError([&](const NetworkErrorContext &context) -> void { result.error = context; })
                .onFinal([&]() -> void { result.final = true; });
            auto options = TlsClientConnectOptions{};
            options.setConfigurationLabel("tls/client/interop"_el)
                .setAlpnProtocols({alpn()})
                .setBufferLimits(
                    SocketBufferLimits{unit::ByteLength{8U * 1024U * 1024U}, unit::ByteLength{8U * 1024U * 1024U}})
                .setHandshakeTimeout(el::time::TimeDelta::seconds(10))
                .setIdleTimeout(el::time::TimeDelta::seconds(10))
                .setCloseTimeout(el::time::TimeDelta::seconds(10));
            connection->connect(
                HostEndpoint{HostName::fromStringOrThrow("localhost"_el), Port{peer.port()}}, std::move(options));
        });
        runUntilFinal(loop, result);
        const auto peerBytes = peer.finish();
        REQUIRE_EQUAL(peerBytes, payloadLength);
        return result;
    }

    void testSuccessfulCipher(const std::string &cipher, const TlsCipherSuite expectedSuite) {
        const auto result = runScenario("tls_echo", cipher, 4U, true);
        REQUIRE(result.handshakeCompleted);
        REQUIRE(result.closed);
        REQUIRE_FALSE(result.sendClosed);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE_EQUAL(result.sent, std::uint64_t{4U});
        REQUIRE_EQUAL(mem::ByteBlock{result.received}, mem::ByteBlock({0x5aU, 0x5aU, 0x5aU, 0x5aU}));
        REQUIRE_EQUAL(result.cipherSuite, std::optional<TlsCipherSuite>{expectedSuite});
        REQUIRE_EQUAL(result.alpn, alpn());
    }

    auto runFailure(const std::string &scenario) -> Result {
        auto peer = InteropEnvironment::instance().startScenario(scenario, "", 0U);
        const auto applicationScope = ApplicationTestScope<>{};
        installTlsConfiguration();
        const auto loop = EventLoop::create();
        auto result = Result{};
        auto connection = TlsClientConnectionPtr{};
        loop->invoke([&]() -> void {
            connection = loop->get<Network>().createTlsClientConnection();
            connection->events()
                .onHandshakeCompleted([&]() -> void { result.handshakeCompleted = true; })
                .onError([&](const NetworkErrorContext &context) -> void { result.error = context; })
                .onFinal([&]() -> void { result.final = true; });
            auto options = TlsClientConnectOptions{};
            options.setConfigurationLabel("tls/client/interop"_el)
                .setHandshakeTimeout(el::time::TimeDelta::seconds(10));
            connection->connect(
                HostEndpoint{HostName::fromStringOrThrow("localhost"_el), Port{peer.port()}}, std::move(options));
        });
        runUntilFinal(loop, result);
        REQUIRE_EQUAL(peer.finish(), std::uint64_t{0U});
        return result;
    }

public:
    void testAes128GcmSha256() { testSuccessfulCipher("TLS_AES_128_GCM_SHA256", TlsCipherSuite::Aes128GcmSha256); }

    void testAes256GcmSha384() { testSuccessfulCipher("TLS_AES_256_GCM_SHA384", TlsCipherSuite::Aes256GcmSha384); }

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
        const auto result = runScenario("truncate_after_data", "TLS_AES_128_GCM_SHA256", 4U, false);
        REQUIRE(result.handshakeCompleted);
        REQUIRE_FALSE(result.closed);
        REQUIRE(result.error.has_value());
        REQUIRE_EQUAL(result.error->reason(), NetworkErrorReason::TlsTruncation);
        REQUIRE_EQUAL(result.error->phase(), NetworkErrorPhase::Active);
        REQUIRE_EQUAL(result.received.length(), unit::ByteLength{4U});
    }

    void testFourMegabyteDuplexStream() {
        constexpr auto cPayloadLength = std::uint64_t{4U * 1024U * 1024U};
        const auto result = runScenario("tls_echo", "TLS_AES_128_GCM_SHA256", cPayloadLength, true);
        REQUIRE(result.handshakeCompleted);
        REQUIRE(result.closed);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE_EQUAL(result.sent, cPayloadLength);
        REQUIRE_EQUAL(result.received.length(), unit::ByteLength{cPayloadLength});
        const auto data = mem::ByteBlock{result.received};
        for (const auto value : data.span()) {
            REQUIRE_EQUAL(value, std::uint8_t{0x5aU});
        }
    }
};
