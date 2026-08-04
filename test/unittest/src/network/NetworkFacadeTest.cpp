// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/tcp/TcpConnection.hpp>
#include <erbsland/network/tcp/TcpListener.hpp>
#include <erbsland/network/tls/TlsClientConnection.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::event;
using namespace el::network;

TESTED_TARGETS(Network TcpConnection TcpListener)
class NetworkFacadeTest final : public el::UnitTest {
public:
    void testDefaultBackendCreatesInactiveSourcesOnOwnerLoop() {
        const auto loop = EventLoop::create();
        auto connection = TcpConnectionPtr{};
        auto listener = TcpListenerPtr{};
        auto tlsConnection = TlsClientConnectionPtr{};
        loop->invoke([&]() -> void {
            auto &network = loop->get<Network>();
            connection = network.createTcpConnection();
            listener = network.createTcpListener();
            tlsConnection = network.createTlsClientConnection();
            REQUIRE_EQUAL(connection->state(), NetworkSourceState::Inactive);
            REQUIRE_EQUAL(listener->state(), NetworkSourceState::Inactive);
            REQUIRE_FALSE(connection->localEndpoint().has_value());
            REQUIRE_FALSE(connection->remoteEndpoint().has_value());
            REQUIRE_FALSE(listener->localEndpoint().has_value());
            REQUIRE_EQUAL(tlsConnection->state(), TlsClientConnectionState::Inactive);
        });
        REQUIRE(loop->runOnce());
        REQUIRE(connection);
        REQUIRE(listener);
        REQUIRE(tlsConnection);
    }

    void testFactoriesRequireOwnerLoop() {
        const auto loop = EventLoop::create();
        auto &network = loop->get<Network>();
        REQUIRE_THROWS(network.createTcpConnection());
        REQUIRE_THROWS(network.createTcpListener());
        REQUIRE_THROWS(network.createTlsClientConnection());
    }
};
