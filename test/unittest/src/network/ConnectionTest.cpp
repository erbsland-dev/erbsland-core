// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/network/source/Connection.hpp>
#include <erbsland/network/source/ConnectionCloseContext.hpp>
#include <erbsland/network/tcp/TcpConnection.hpp>
#include <erbsland/network/tls/TlsClientConnection.hpp>
#include <erbsland/network/tls/TlsServerConnection.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <concepts>

using namespace el::network;

TESTED_TARGETS(Connection ConnectionEventEditor ConnectionState ConnectionCloseContext ConnectionCloseOrigin)
class ConnectionTest final : public el::UnitTest {
public:
    void testAllProtocolConnectionsShareTheApplicationStreamInterface() {
        static_assert(std::derived_from<TcpConnection, Connection>);
        static_assert(std::derived_from<TlsClientConnection, Connection>);
        static_assert(std::derived_from<TlsServerConnection, Connection>);
        static_assert(std::derived_from<TcpConnectionEventEditor, ConnectionEventEditor>);
        static_assert(std::derived_from<TlsClientConnectionEventEditor, ConnectionEventEditor>);
        static_assert(std::derived_from<TlsServerConnectionEventEditor, ConnectionEventEditor>);
        REQUIRE_EQUAL(ConnectionCloseContext{ConnectionCloseOrigin::Local}.origin(), ConnectionCloseOrigin::Local);
        REQUIRE_EQUAL(ConnectionCloseContext{ConnectionCloseOrigin::Remote}.origin(), ConnectionCloseOrigin::Remote);
    }
};
