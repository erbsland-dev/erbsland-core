// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/network/source/ConnectionCloseContext.hpp>
#include <erbsland/network/tcp/TcpAcceptOptions.hpp>
#include <erbsland/network/tcp/TcpConnectionFilterResult.hpp>
#include <erbsland/network/tcp/TcpConnectOptions.hpp>
#include <erbsland/network/tcp/TcpListenerOptions.hpp>
#include <erbsland/network/udp/UdpDatagram.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::network;

TESTED_TARGETS(
    TcpAcceptOptions TcpConnectOptions ConnectionCloseContext TcpConnectionFilterResult TcpListenerOptions UdpDatagram)
class NetworkProtocolFacadeTest final : public el::UnitTest {
public:
    void testTcpOptionDefaultsAndSetters() {
        auto connect = TcpConnectOptions{};
        REQUIRE_EQUAL(connect.timeout(), TcpConnectOptions::cDefaultTimeout);
        REQUIRE_EQUAL(connect.bufferLimits().send(), el::unit::ByteLength{1024U * 1024U});
        const auto limits = SocketBufferLimits{el::unit::ByteLength{4096U}, el::unit::ByteLength{8192U}};
        connect.setTimeout(el::time::TimeDelta::seconds(5)).setBufferLimits(limits);
        REQUIRE_EQUAL(connect.timeout(), el::time::TimeDelta::seconds(5));
        REQUIRE_EQUAL(connect.bufferLimits().receive(), el::unit::ByteLength{8192U});

        auto accept = TcpAcceptOptions{};
        accept.setBufferLimits(limits);
        REQUIRE_EQUAL(accept.bufferLimits().send(), el::unit::ByteLength{4096U});

        auto listener = TcpListenerOptions{};
        REQUIRE_EQUAL(listener.backlog(), el::unit::ItemCount{128U});
        REQUIRE_EQUAL(listener.maximumPendingRequests(), el::unit::ItemCount{128U});
        listener.setBacklog(el::unit::ItemCount{32U}).setMaximumPendingRequests(el::unit::ItemCount{8U});
        REQUIRE_EQUAL(listener.backlog(), el::unit::ItemCount{32U});
        REQUIRE_EQUAL(listener.maximumPendingRequests(), el::unit::ItemCount{8U});
    }

    void testCloseContextAndFilterResult() {
        const auto context = ConnectionCloseContext{ConnectionCloseOrigin::Remote};
        REQUIRE_EQUAL(context.origin(), ConnectionCloseOrigin::Remote);
        REQUIRE_NOT_EQUAL(TcpConnectionFilterResult::Accept, TcpConnectionFilterResult::Reject);
    }
};
