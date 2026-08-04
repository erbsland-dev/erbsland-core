// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/configuration/CryptologyConfiguration.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/cryptology/tls/TlsConfiguration.hpp>
#include <erbsland/cryptology/tls/TlsServerIdentity.hpp>
#include <erbsland/cryptology/x509/X509CertificateBundle.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/network/source/ConnectionQuota.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/source/NetworkErrorContext.hpp>
#include <erbsland/network/tcp/TcpConnectionRequest.hpp>
#include <erbsland/network/tcp/TcpListener.hpp>
#include <erbsland/network/tls/TlsServerConnection.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/system/Subprocess.hpp>
#include <erbsland/system/SubprocessOptions.hpp>
#include <erbsland/system/SubprocessOutputMode.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace el::cryptology;
using namespace el::event;
using namespace el::network;
using namespace el::text::literals;
namespace mem = el::mem;
namespace unit = el::unit;

TESTED_TARGETS(TlsServerConnection TlsServerProtocol)
class ExternalTlsServerInteropTest final : public el::UnitTest {
private:
    struct Result final {
        bool handshakeCompleted{};
        bool final{};
        bool responseSent{};
        std::optional<NetworkErrorContext> error;
    };

    [[nodiscard]] static auto toString(const std::string &value) -> el::text::String {
        return el::text::StringConverter{value}.toString();
    }

    [[nodiscard]] static auto readText(const char *path) -> el::text::String {
        return el::text::String{el::unittest::fh::readDataText(path)};
    }

    static void installIdentity() {
        auto configuration = TlsConfiguration{};
        configuration.setServerIdentity(TlsServerIdentity{
            X509CertificateBundle::fromPemOrThrow(readText("data/network/tls-interop/server.pem")),
            SigningPrivateKey::fromPemOrThrow(readText("data/network/tls-interop/server-key.pem"))});
        el::core::application().cryptologyConfiguration().setTlsConfiguration(
            "tls/server/external"_el, std::move(configuration));
    }

    [[nodiscard]] static auto alpn() -> mem::ByteBlock {
        return mem::ByteBlock({'e', 'r', 'b', 's', 'l', 'a', 'n', 'd', '-', 't', 'e', 's', 't'});
    }

    [[nodiscard]] static auto httpResponse() -> mem::ByteBlock {
        constexpr auto cText = std::string_view{
            "HTTP/1.1 200 OK\r\nContent-Length: 12\r\nConnection: close\r\n\r\nhello curl!\n"};
        auto result = mem::ByteBlockEditor{};
        for (const auto character : cText) {
            result.append(static_cast<mem::Byte>(character));
        }
        return result;
    }

    [[nodiscard]] static auto captureOptions() -> el::system::SubprocessOptions {
        return el::system::SubprocessOptions{}
            .setInheritStandardInput(false)
            .setStandardOutputMode(el::system::SubprocessOutputMode::Capture)
            .setStandardErrorMode(el::system::SubprocessOutputMode::Capture);
    }

    [[nodiscard]] static auto curlArguments(const std::uint16_t port) -> el::text::StringList {
        const auto authority = "localhost:" + std::to_string(port);
        auto arguments = el::text::StringList{
            "--tlsv1.3"_el,
            "--tls-max"_el,
            "1.3"_el,
            "--cacert"_el,
            toString(ERBSLAND_CORE_INTEROP_CA_FILE)};
#if defined(_WIN32)
        // The standalone fixture intentionally has no online revocation service. Schannel otherwise rejects it
        // before exercising the TLS implementation under test.
        arguments.append("--ssl-no-revoke"_el);
#endif
        arguments.append("--resolve"_el)
            .append(toString(authority + ":127.0.0.1"))
            .append("--http1.1"_el)
            .append("--silent"_el)
            .append("--show-error"_el)
            .append(toString("https://" + authority + "/"));
        return arguments;
    }

    auto runServer(
        const bool http,
        const std::function<el::system::Subprocess(std::uint16_t)> &startProcess) ->
        std::pair<el::system::Subprocess, Result> {
        const auto applicationScope = ApplicationTestScope<>{};
        installIdentity();
        const auto loop = EventLoop::create();
        const auto tcpQuota = ConnectionQuota::create(unit::ItemCount{2U});
        const auto handshakeQuota = ConnectionQuota::create(unit::ItemCount{1U});
        auto listener = TcpListenerPtr{};
        auto connection = TlsServerConnectionPtr{};
        auto listenerReady = false;
        auto result = Result{};
        loop->invoke([&]() -> void {
            listener = loop->get<Network>().createTcpListener();
            listener->events()
                .onListening([&]() -> void { listenerReady = true; })
                .onConnection([&](TcpConnectionRequestPtr request) -> void {
                    connection = loop->get<Network>().createTlsServerConnection();
                    connection->events()
                        .onHandshakeCompleted([&]() -> void {
                            result.handshakeCompleted = true;
                            if (!http) {
                                loop->invoke([&]() -> void {
                                    REQUIRE(connection->send(mem::ByteBlock({
                                        'o', 'p', 'e', 'n', 's', 's', 'l', '-', 'o', 'k', '\n'})).isAccepted());
                                    connection->close();
                                });
                            }
                        })
                        .onData([&](mem::ByteBlock) -> void {
                            if (http && !result.responseSent) {
                                result.responseSent = true;
                                loop->invoke([&]() -> void {
                                    REQUIRE(connection->send(httpResponse()).isAccepted());
                                    connection->close();
                                });
                            }
                        })
                        .onError([&](const NetworkErrorContext &context) -> void { result.error = context; })
                        .onFinal([&]() -> void { result.final = true; });
                    auto options = TlsServerAcceptOptions{handshakeQuota};
                    options.setConfigurationLabel("tls/server/external"_el)
                        .setAlpnProtocols({http ? mem::ByteBlock({'h', 't', 't', 'p', '/', '1', '.', '1'}) : alpn()});
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

        auto process = startProcess(listener->localEndpoint()->port().toRawValue());
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{20};
        while ((!result.final || process.isRunning()) && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(el::time::Milliseconds{20}));
        }
        REQUIRE(result.final);
        REQUIRE_FALSE(process.isRunning());
        REQUIRE_EQUAL(tcpQuota->current(), unit::ItemCount{});
        REQUIRE_EQUAL(handshakeQuota->current(), unit::ItemCount{});
        return {std::move(process), std::move(result)};
    }

public:
    void testOpenSslSClientTls13SniAlpnStreamAndClosure() {
#if defined(ERBSLAND_CORE_OPENSSL_EXECUTABLE)
        const auto [process, result] = runServer(false, [](const std::uint16_t port) -> el::system::Subprocess {
            return el::system::Subprocess::start(
                el::path::Path{std::filesystem::path{ERBSLAND_CORE_OPENSSL_EXECUTABLE}},
                el::text::StringList{
                    "s_client"_el,
                    "-connect"_el,
                    toString("127.0.0.1:" + std::to_string(port)),
                    "-servername"_el,
                    "localhost"_el,
                    "-alpn"_el,
                    "erbsland-test"_el,
                    "-tls1_3"_el,
                    "-CAfile"_el,
                    toString(ERBSLAND_CORE_INTEROP_CA_FILE),
                    "-quiet"_el},
                captureOptions());
        });
        REQUIRE(result.handshakeCompleted);
        REQUIRE_FALSE(result.error.has_value());
        REQUIRE(process.exitStatus()->isSuccess());
        REQUIRE(process.standardOutput().contains("openssl-ok"_el));
#endif
    }

    void testCurlHttpsWithManualHttp11Response() {
#if defined(ERBSLAND_CORE_CURL_EXECUTABLE)
        const auto [process, result] = runServer(true, [](const std::uint16_t port) -> el::system::Subprocess {
            return el::system::Subprocess::start(
                el::path::Path{std::filesystem::path{ERBSLAND_CORE_CURL_EXECUTABLE}},
                curlArguments(port), captureOptions());
        });
        if (!process.exitStatus()->isSuccess()) {
            std::cerr << "curl TLS interop error: "
                      << el::text::StringConverter{process.standardError()}.toStdString() << '\n';
        }
        REQUIRE(result.handshakeCompleted);
        REQUIRE(result.responseSent);
        REQUIRE(process.exitStatus()->isSuccess());
        REQUIRE_EQUAL(process.standardOutput(), "hello curl!\n"_el);
        if (result.error.has_value()) {
            // curl may close its native socket as soon as the declared HTTP body is complete instead of waiting for
            // the server's close_notify exchange. Accept only that post-response closing reset.
            REQUIRE_EQUAL(result.error->reason(), NetworkErrorReason::ConnectionReset);
            REQUIRE_EQUAL(result.error->phase(), NetworkErrorPhase::Closing);
        }
#endif
    }
};
