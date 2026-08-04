// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/event/EventLoop.hpp>
#include <erbsland/network/Network.hpp>
#include <erbsland/network/source/NetworkErrorContext.hpp>
#include <erbsland/network/tcp/TcpConnection.hpp>
#include <erbsland/network/tcp/TcpConnectionCloseContext.hpp>
#include <erbsland/network/tcp/TcpConnectionRequest.hpp>
#include <erbsland/network/tcp/TcpListener.hpp>
#include <erbsland/time/TimeDelta.hpp>
#include <erbsland/time/TimeUnitTags.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <chrono>
#include <optional>

using namespace el::event;
using namespace el::network;
using namespace el::time;

TESTED_TARGETS(TcpConnection TcpListener TcpConnectionRequest TcpConnectionCloseContext)
class TcpSocketLiveTest final : public el::UnitTest {
public:
    void testIpv4LoopbackConnectionAndCloseOrigins() { runLoopbackConnection(IpAddress::loopbackV4()); }

    void testIpv6LoopbackConnectionAndCloseOrigins() { runLoopbackConnection(IpAddress::loopbackV6()); }

    void testConnectionRefusal() {
        const auto loop = EventLoop::create();
        auto connection = el::network::TcpConnectionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto final = false;
        loop->invoke([&]() -> void {
            connection = loop->get<Network>().createTcpConnection();
            connection->events()
                .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                .onFinal([&]() -> void { final = true; });
            connection->connect(HostEndpoint{IpAddress::loopbackV4(), Port{9U}});
        });

        runUntil(loop, [&]() -> bool { return final; }, std::chrono::seconds{10});
        REQUIRE(error.has_value());
        REQUIRE_EQUAL(error->reason(), NetworkErrorReason::ConnectionRefused);
    }

    void testListenerAddressConflict() {
        const auto loop = EventLoop::create();
        auto first = el::network::TcpListenerPtr{};
        auto second = el::network::TcpListenerPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto final = false;
        loop->invoke([&]() -> void {
            auto &network = loop->get<Network>();
            first = network.createTcpListener();
            first->events().onListening([&]() -> void {
                second = loop->get<Network>().createTcpListener();
                second->events()
                    .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                    .onFinal([&]() -> void { final = true; });
                second->start(*first->localEndpoint());
            });
            first->start(IpEndpoint{IpAddress::loopbackV4(), Port{}});
        });

        runUntil(loop, [&]() -> bool { return final; });
        REQUIRE(error.has_value());
        REQUIRE_EQUAL(error->reason(), NetworkErrorReason::AddressInUse);
        loop->invoke([&]() -> void { first->close(); });
        static_cast<void>(loop->runUntilIdle());
    }

    void testHostResolvedAbortPreventsNativeConnection() {
        const auto loop = EventLoop::create();
        auto connection = el::network::TcpConnectionPtr{};
        auto resolved = false;
        auto connected = false;
        auto final = false;
        loop->invoke([&]() -> void {
            connection = loop->get<Network>().createTcpConnection();
            connection->events()
                .onHostResolved([&](const el::util::List<IpEndpoint> &) -> void {
                    resolved = true;
                    connection->abort();
                })
                .onConnected([&]() -> void { connected = true; })
                .onFinal([&]() -> void { final = true; });
            connection->connect(HostEndpoint{IpAddress::loopbackV4(), Port{9U}});
        });
        runUntil(loop, [&]() -> bool { return final; });
        REQUIRE(resolved);
        REQUIRE_FALSE(connected);
        REQUIRE_EQUAL(connection->state(), NetworkSourceState::Closed);
    }

private:
    void runLoopbackConnection(const IpAddress &address) {
        const auto loop = EventLoop::create();
        auto listener = el::network::TcpListenerPtr{};
        auto client = el::network::TcpConnectionPtr{};
        auto server = el::network::TcpConnectionPtr{};
        auto error = std::optional<NetworkErrorContext>{};
        auto resolved = false;
        auto echoed = false;
        auto clientFinal = false;
        auto serverFinal = false;
        auto clientOrigin = std::optional<TcpConnectionCloseOrigin>{};
        auto serverOrigin = std::optional<TcpConnectionCloseOrigin>{};
        const auto payload = el::mem::ByteBlock{el::unit::ByteLength{32U}, el::mem::Byte{0x5aU}};

        loop->invoke([&]() -> void {
            auto &network = loop->get<Network>();
            listener = network.createTcpListener();
            listener->events()
                .onListening([&]() -> void {
                    client = loop->get<Network>().createTcpConnection();
                    client->events()
                        .onHostResolved([&](const el::util::List<IpEndpoint> &endpoints) -> void {
                            resolved = true;
                            REQUIRE_EQUAL(endpoints.count(), el::unit::ItemCount{1U});
                            REQUIRE_EQUAL(endpoints.first(), *listener->localEndpoint());
                        })
                        .onConnected([&]() -> void { REQUIRE(client->send(payload).isAccepted()); })
                        .onData([&](el::mem::ByteBlock data) -> void {
                            REQUIRE_EQUAL(data, payload);
                            echoed = true;
                            client->close();
                        })
                        .onClosed(
                            [&](const TcpConnectionCloseContext &context) -> void { clientOrigin = context.origin(); })
                        .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                        .onFinal([&]() -> void { clientFinal = true; });
                    const auto endpoint = *listener->localEndpoint();
                    client->connect(HostEndpoint{endpoint.address(), endpoint.port(), endpoint.scopeId()});
                })
                .onConnection([&](el::network::TcpConnectionRequestPtr request) -> void {
                    server = loop->get<Network>().createTcpConnection();
                    server->events()
                        .onConnected([]() -> void {})
                        .onData([&](el::mem::ByteBlock data) -> void { REQUIRE(server->send(data).isAccepted()); })
                        .onClosed(
                            [&](const TcpConnectionCloseContext &context) -> void { serverOrigin = context.origin(); })
                        .onError([&](const NetworkErrorContext &context) -> void { error = context; })
                        .onFinal([&]() -> void { serverFinal = true; });
                    server->accept(std::move(request));
                })
                .onError([&](const NetworkErrorContext &context) -> void { error = context; });
            listener->start(IpEndpoint{address, Port{}});
        });

        runUntil(loop, [&]() -> bool { return (clientFinal && serverFinal) || error.has_value(); });
        REQUIRE_FALSE(error.has_value());
        REQUIRE(resolved);
        REQUIRE(echoed);
        REQUIRE_EQUAL(clientOrigin, std::optional<TcpConnectionCloseOrigin>{TcpConnectionCloseOrigin::Local});
        REQUIRE_EQUAL(serverOrigin, std::optional<TcpConnectionCloseOrigin>{TcpConnectionCloseOrigin::Remote});

        loop->invoke([&]() -> void { listener->close(); });
        static_cast<void>(loop->runUntilIdle());
    }

    template <typename Predicate>
    void runUntil(
        const EventLoopPtr &loop,
        Predicate predicate,
        const std::chrono::steady_clock::duration timeout = std::chrono::seconds{3}) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!predicate() && std::chrono::steady_clock::now() < deadline) {
            static_cast<void>(loop->runOnce(TimeDelta{Milliseconds{25}}));
        }
        REQUIRE(predicate());
    }
};
