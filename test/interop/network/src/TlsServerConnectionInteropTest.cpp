// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "InteropEnvironment.hpp"

#include "core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/tls/TlsCipherSuite.hpp>
#include <erbsland/cryptology/tls/TlsConfiguration.hpp>
#include <erbsland/cryptology/tls/TlsServerIdentity.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/source/ConnectionQuota.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/source/NetworkErrorContext.hpp>
#include <erbsland/network/tcp/TcpConnectionRequest.hpp>
#include <erbsland/network/tcp/TcpListener.hpp>
#include <erbsland/network/tls/TlsServerConnection.hpp>
#include <erbsland/network/tls/TlsServerConnectionCloseContext.hpp>
#include <erbsland/network/tls/TlsServerConnectionCloseOrigin.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

using namespace el::cryptology;
using namespace el::event;
using namespace el::network;
using namespace el::text::literals;
namespace mem = el::mem;
namespace unit = el::unit;

TESTED_TARGETS(TlsServerConnection TlsServerProtocol TcpListener ConnectionQuota ConnectionQuotaLease)
class TlsServerConnectionInteropTest final : public el::UnitTest {
private:
    struct Result final {
        bool clientHello{};
        bool handshakeCompleted{};
        bool closed{};
        bool final{};
        std::uint64_t bytesReceived{};
        std::optional<NetworkErrorContext> error;
        std::optional<TlsServerConnectionCloseOrigin> closeOrigin;
        std::optional<TlsCipherSuite> cipherSuite;
        el::text::String requestedLabel;
        mem::ByteBlock alpn;
    };

    [[nodiscard]] static auto readText(const char *path) -> el::text::String {
        return el::text::String{el::unittest::fh::readDataText(path)};
    }

    static void installIdentity(const el::text::String &label, const char *certificatePath, const char *keyPath) {
        auto configuration = TlsConfiguration{};
        configuration.setServerIdentity(TlsServerIdentity{
            X509CertificateBundle::fromPemOrThrow(readText(certificatePath)),
            SigningPrivateKey::fromPemOrThrow(readText(keyPath))});
        el::core::application().cryptologyConfiguration().setTlsConfiguration(label, std::move(configuration));
    }

    [[nodiscard]] static auto alpn() -> mem::ByteBlock {
        return mem::ByteBlock({'e', 'r', 'b', 's', 'l', 'a', 'n', 'd', '-', 't', 'e', 's', 't'});
    }

    void runUntilFinal(const EventLoopPtr &loop, Result &result) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{30};
        while (!result.final && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(el::time::Milliseconds{20}));
        }
        REQUIRE(result.final);
    }

    auto runScenario(
        const char *certificatePath,
        const char *keyPath,
        const std::string &cipher,
        const std::uint64_t payloadLength,
        const bool truncate,
        const bool exactMapping = false) -> Result {
        const auto applicationScope = ApplicationTestScope<>{};
        installIdentity("tls/server/interop/default"_el, certificatePath, keyPath);
        if (exactMapping) {
            installIdentity("tls/server/interop/exact"_el, certificatePath, keyPath);
        }
        const auto loop = EventLoop::create();
        const auto tcpQuota = ConnectionQuota::create(unit::ItemCount{4U});
        const auto handshakeQuota = ConnectionQuota::create(unit::ItemCount{2U});
        auto listener = TcpListenerPtr{};
        auto connection = TlsServerConnectionPtr{};
        auto listenerReady = false;
        auto result = Result{};
        loop->invoke([&]() -> void {
            auto &network = loop->get<Network>();
            listener = network.createTcpListener();
            listener->events()
                .onListening([&]() -> void { listenerReady = true; })
                .onConnection([&](TcpConnectionRequestPtr request) -> void {
                    connection = loop->get<Network>().createTlsServerConnection();
                    connection->events()
                        .onClientHello([&]() -> void {
                            result.clientHello = true;
                            result.requestedLabel = *connection->requestedConfigurationLabel();
                            REQUIRE_EQUAL(
                                connection->serverName()->toString(HostNameFormat::IdnaAscii), "localhost"_el);
                        })
                        .onHandshakeCompleted([&]() -> void {
                            result.handshakeCompleted = true;
                            result.cipherSuite = connection->cipherSuite();
                            result.alpn = connection->negotiatedAlpn();
                        })
                        .onData([&](mem::ByteBlock data) -> void {
                            result.bytesReceived += data.length().toSizeT();
                            REQUIRE(connection->send(data).isAccepted());
                        })
                        .onClosed([&](const TlsServerConnectionCloseContext &context) -> void {
                            result.closed = true;
                            result.closeOrigin = context.origin();
                        })
                        .onError([&](const NetworkErrorContext &context) -> void { result.error = context; })
                        .onFinal([&]() -> void { result.final = true; });
                    auto options = TlsServerAcceptOptions{handshakeQuota};
                    options.setConfigurationLabel("tls/server/interop/default"_el)
                        .setAlpnProtocols({alpn()})
                        .setBufferLimits(SocketBufferLimits{
                            unit::ByteLength{8U * 1024U * 1024U}, unit::ByteLength{8U * 1024U * 1024U}})
                        .setHandshakeTimeout(el::time::TimeDelta::seconds(10))
                        .setIdleTimeout(el::time::TimeDelta::seconds(10))
                        .setCloseTimeout(el::time::TimeDelta::seconds(10));
                    if (exactMapping) {
                        options.setIdentityMappings(
                            {{HostName::fromStringOrThrow("LOCALHOST"_el), "tls/server/interop/exact"_el}});
                    }
                    connection->accept(std::move(request), std::move(options));
                    listener->close();
                })
                .onError([&](const NetworkErrorContext &context) -> void { result.error = context; });
            listener->start(
                IpEndpoint{IpAddress::loopbackV4(), Port{}}, TcpListenerOptions{}.setConnectionQuota(tcpQuota));
        });
        while (!listenerReady) {
            static_cast<void>(loop->runOnce(el::time::Milliseconds{20}));
        }
        auto peer = InteropEnvironment::instance().startClientScenario(
            listener->localEndpoint()->port().toRawValue(),
            truncate ? "tls_client_truncate" : "tls_client_echo",
            cipher,
            payloadLength);
        runUntilFinal(loop, result);
        if (result.error.has_value() && !truncate) {
            std::cerr << "TLS server interop error: "
                      << el::text::StringConverter{result.error->description()}.toStdString() << '\n';
        }
        REQUIRE_EQUAL(peer.finish(), payloadLength);
        REQUIRE_EQUAL(tcpQuota->current(), unit::ItemCount{});
        REQUIRE_EQUAL(handshakeQuota->current(), unit::ItemCount{});
        return result;
    }

    void testSuccessfulCipher(const std::string &cipher, const TlsCipherSuite expectedSuite) {
        const auto result = runScenario(
            "data/network/tls-interop/server.pem",
            "data/network/tls-interop/server-key.pem",
            cipher,
            4U,
            false);
        REQUIRE(result.clientHello);
        REQUIRE(result.handshakeCompleted);
        REQUIRE(result.closed);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE_EQUAL(result.bytesReceived, std::uint64_t{4U});
        REQUIRE_EQUAL(result.cipherSuite, std::optional<TlsCipherSuite>{expectedSuite});
        REQUIRE_EQUAL(result.alpn, alpn());
        REQUIRE_EQUAL(result.closeOrigin, std::optional<TlsServerConnectionCloseOrigin>{TlsServerConnectionCloseOrigin::Remote});
    }

public:
    void testAes128GcmSha256() { testSuccessfulCipher("TLS_AES_128_GCM_SHA256", TlsCipherSuite::Aes128GcmSha256); }

    void testAes256GcmSha384() { testSuccessfulCipher("TLS_AES_256_GCM_SHA384", TlsCipherSuite::Aes256GcmSha384); }

    void testChaCha20Poly1305Sha256() {
        testSuccessfulCipher("TLS_CHACHA20_POLY1305_SHA256", TlsCipherSuite::ChaCha20Poly1305Sha256);
    }

    void testExactEcdsaIdentitySelection() {
        const auto result = runScenario(
            "data/network/tls-interop/server-ecdsa.pem",
            "data/network/tls-interop/server-ecdsa-key.pem",
            "TLS_AES_128_GCM_SHA256",
            4U,
            false,
            true);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE_EQUAL(result.requestedLabel, "tls/server/interop/exact"_el);
    }

    void testEd25519Identity() {
        const auto result = runScenario(
            "data/network/tls-interop/server-ed25519.pem",
            "data/network/tls-interop/server-ed25519-key.pem",
            "TLS_AES_128_GCM_SHA256",
            4U,
            false);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE(result.handshakeCompleted);
    }

    void testTruncatedActiveConnection() {
        const auto result = runScenario(
            "data/network/tls-interop/server.pem",
            "data/network/tls-interop/server-key.pem",
            "TLS_AES_128_GCM_SHA256",
            4U,
            true);
        REQUIRE(result.handshakeCompleted);
        REQUIRE_FALSE(result.closed);
        REQUIRE(result.error.has_value());
        REQUIRE_EQUAL(result.error->reason(), NetworkErrorReason::TlsTruncation);
        REQUIRE_EQUAL(result.error->phase(), NetworkErrorPhase::Active);
    }

    void testFourMegabyteDuplexAndKeyUpdate() {
        constexpr auto cPayloadLength = std::uint64_t{4U * 1024U * 1024U};
        const auto result = runScenario(
            "data/network/tls-interop/server.pem",
            "data/network/tls-interop/server-key.pem",
            "TLS_AES_128_GCM_SHA256",
            cPayloadLength,
            false);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE_EQUAL(result.bytesReceived, cPayloadLength);
    }
};
