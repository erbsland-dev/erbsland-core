// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteSpan.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocol_fwd.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocol_fwd.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocolTestAccess_fwd.hpp>

#include <cstdint>
#include <span>

/// Exercise the TLS server protocol for one fuzz input.
class TlsServerProtocolFuzz final {
private:
    using ClientProtocol = erbsland::network::impl::TlsClientProtocol;
    using ServerProtocol = erbsland::network::impl::TlsServerProtocol;
    using ServerTestAccess = erbsland::network::impl::TlsServerProtocolTestAccess;

public:
    /// Create a harness for one input.
    /// @param data The bytes supplied by libFuzzer.
    explicit TlsServerProtocolFuzz(const std::span<const uint8_t> data) : _data{data} {}

public: // operations
    /// Process the input once.
    /// @return Zero to continue fuzzing.
    [[nodiscard]] auto run() const -> int;

private:
    /// Create the client used to drive the fixture handshake.
    [[nodiscard]] static auto createClient(bool mutual) -> ClientProtocol;

    /// Create the server protocol under test.
    [[nodiscard]] static auto createServer(bool mutual) -> ServerProtocol;

    /// Drive the fixture handshake to the established state.
    [[nodiscard]] static auto establish(ClientProtocol &client, ServerProtocol &server) -> bool;

    /// Deliver pending client output to the server.
    [[nodiscard]] static auto transferClientOutput(ClientProtocol &client, ServerProtocol &server) -> bool;

    /// Deliver pending server output to the client.
    [[nodiscard]] static auto transferServerOutput(ServerProtocol &server, ClientProtocol &client) -> bool;

    /// Feed bytes to the protocol in fragments.
    static void feedFragmented(
        ServerProtocol &protocol, const erbsland::mem::ConstByteSpan data, const uint8_t selector);

private:
    std::span<const uint8_t> _data; ///< Mode/fragment selector followed by bounded arbitrary bytes.
};
