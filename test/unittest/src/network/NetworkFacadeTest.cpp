// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/http_client/HttpClientSession.hpp>
#include <erbsland/network/http_client/HttpClientSessionOptions.hpp>
#include <erbsland/network/http_client/HttpClientTlsOptions.hpp>
#include <erbsland/network/http_server/HttpServer.hpp>
#include <erbsland/network/http_server/HttpServerRouteOptions.hpp>
#include <erbsland/network/http_server/HttpServerTlsOptions.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/tcp/TcpConnection.hpp>
#include <erbsland/network/tcp/TcpListener.hpp>
#include <erbsland/network/tls/TlsClientConnection.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::event;
using namespace el::network;
using namespace el::text::literals;

TESTED_TARGETS(
    Network TcpConnection TcpListener HttpServer HttpClientSession HttpServerTlsOptions HttpServerRouteOptions)
class NetworkFacadeTest final : public el::UnitTest {
public:
    void testHttpServerConvenienceOptionDefaults() {
        const auto tls = HttpServerTlsOptions{};
        REQUIRE_EQUAL(tls.configurationLabel(), "http/server"_el);
        REQUIRE_EQUAL(tls.maximumConcurrentHandshakes(), el::unit::ItemCount{128U});
        REQUIRE_EQUAL(tls.handshakeTimeout(), el::time::TimeDelta::seconds(30));
        REQUIRE(tls.identityMappings().empty());

        const auto route = HttpServerRouteOptions{};
        REQUIRE_EQUAL(route.maximumBodyLength(), el::unit::ByteLength{1024U * 1024U});
        REQUIRE_FALSE(route.acceptedContentTypes().has_value());
    }

    void testHttpClientConvenienceOptionDefaults() {
        const auto options = HttpClientSessionOptions{};
        REQUIRE_EQUAL(options.maximumConcurrentRequests(), el::unit::ItemCount{8U});
        REQUIRE_EQUAL(options.maximumPendingRequests(), el::unit::ItemCount{256U});
        REQUIRE_EQUAL(options.maximumRetainedRequestBodyLength(), el::unit::ByteLength{64U * 1024U * 1024U});
        REQUIRE_EQUAL(options.dnsTimeout(), el::time::TimeDelta::seconds(10));
        REQUIRE_EQUAL(options.connectTimeout(), el::time::TimeDelta::seconds(30));
        REQUIRE_EQUAL(options.headerTimeout(), el::time::TimeDelta::seconds(30));
        REQUIRE_EQUAL(options.bodyIdleTimeout(), el::time::TimeDelta::minutes(5));
        REQUIRE_EQUAL(options.overallTimeout(), el::time::TimeDelta::minutes(30));
        REQUIRE_EQUAL(options.closeTimeout(), el::time::TimeDelta::seconds(10));
        REQUIRE_EQUAL(HttpClientTlsOptions{}.configurationLabel(), "http/client"_el);
    }

    void testDefaultBackendCreatesInactiveSourcesOnOwnerLoop() {
        const auto loop = EventLoop::create();
        auto connection = TcpConnectionPtr{};
        auto listener = TcpListenerPtr{};
        auto httpServer = HttpServerPtr{};
        auto httpClient = HttpClientSessionPtr{};
        auto tlsConnection = TlsClientConnectionPtr{};
        loop->invoke([&]() -> void {
            auto &network = loop->get<Network>();
            connection = network.createTcpConnection();
            listener = network.createTcpListener();
            tlsConnection = network.createTlsClientConnection();
            httpServer = network.createHttpServer();
            httpClient = network.createHttpClientSession();
            REQUIRE_EQUAL(connection->state(), ConnectionState::Inactive);
            REQUIRE_EQUAL(listener->state(), NetworkSourceState::Inactive);
            REQUIRE_FALSE(connection->localEndpoint().has_value());
            REQUIRE_FALSE(connection->remoteEndpoint().has_value());
            REQUIRE_FALSE(listener->localEndpoint().has_value());
            REQUIRE_EQUAL(tlsConnection->state(), ConnectionState::Inactive);
            REQUIRE_EQUAL(httpServer->state(), NetworkSourceState::Inactive);
            REQUIRE_EQUAL(httpClient->state(), NetworkSourceState::Active);
            httpClient->abort();
        });
        REQUIRE(loop->runOnce());
        REQUIRE(connection);
        REQUIRE(listener);
        REQUIRE(tlsConnection);
        REQUIRE(httpServer);
        REQUIRE(httpClient);
    }

    void testFactoriesRequireOwnerLoop() {
        const auto loop = EventLoop::create();
        auto &network = loop->get<Network>();
        REQUIRE_THROWS(network.createTcpConnection());
        REQUIRE_THROWS(network.createTcpListener());
        REQUIRE_THROWS(network.createTlsClientConnection());
        REQUIRE_THROWS(network.createHttpServer());
        REQUIRE_THROWS(network.createHttpClientSession());
    }
};
