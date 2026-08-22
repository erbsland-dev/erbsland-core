// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientConnectOptions_fwd.hpp"

#include "../source/SocketBufferLimits.hpp"
#include "../tcp/TcpConnectOptions.hpp"

#include "../../text/String.hpp"
#include "../../time/TimeDelta.hpp"

#include <utility>
#include <vector>

namespace erbsland::network {

/// Options captured when an outgoing TLS client connection is started.
/// @tested{TlsClientConnectionTest}
class TlsClientConnectOptions final {
public:
    /// Reserved framework label for regular TLS clients.
    inline static const auto cDefaultConfigurationLabel = text::String{"tls/client"};
    /// Default handshake deadline after TCP connection establishment.
    inline static const auto cDefaultHandshakeTimeout = time::TimeDelta::seconds(30);
    /// Default authenticated application idle deadline.
    inline static const auto cDefaultIdleTimeout = time::TimeDelta::minutes(5);
    /// Default bidirectional close-notify deadline.
    inline static const auto cDefaultCloseTimeout = time::TimeDelta::seconds(10);
    /// Maximum accepted TLS-owned queue limit in either direction.
    static constexpr auto cMaximumBufferLength = unit::ByteLength{16U * 1024U * 1024U};
    /// Maximum one-shot application send size.
    static constexpr auto cMaximumApplicationSendLength = unit::ByteLength{16U * 1024U};

public:
    /// Get the application TLS configuration label.
    [[nodiscard]] auto configurationLabel() const noexcept -> const text::String & { return _configurationLabel; }
    /// Set the application TLS configuration label.
    /// @param value The exact or descendant registry label to resolve.
    /// @return This options object for chaining.
    auto setConfigurationLabel(text::String value) noexcept -> TlsClientConnectOptions & {
        _configurationLabel = std::move(value);
        return *this;
    }
    /// Get the nested DNS/TCP connection options.
    [[nodiscard]] auto tcpOptions() const noexcept -> const TcpConnectOptions & { return _tcpOptions; }
    /// Set the nested DNS/TCP connection options.
    /// @param value The DNS and TCP options.
    /// @return This options object for chaining.
    auto setTcpOptions(TcpConnectOptions value) noexcept -> TlsClientConnectOptions & {
        _tcpOptions = std::move(value);
        return *this;
    }
    /// Get the TLS protocol queue limits.
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits { return _bufferLimits; }
    /// Set the TLS protocol queue limits.
    /// @param value The TLS-owned send and aggregate receive bounds.
    /// @return This options object for chaining.
    auto setBufferLimits(const SocketBufferLimits value) noexcept -> TlsClientConnectOptions & {
        _bufferLimits = value;
        return *this;
    }
    /// Get the ordered ALPN offers.
    [[nodiscard]] auto alpnProtocols() const noexcept -> const std::vector<text::String> & { return _alpnProtocols; }
    /// Set the ordered ALPN offers.
    /// @param value The protocols in client preference order.
    /// @return This options object for chaining.
    auto setAlpnProtocols(std::vector<text::String> value) noexcept -> TlsClientConnectOptions & {
        _alpnProtocols = std::move(value);
        return *this;
    }
    /// Get the TLS handshake timeout.
    [[nodiscard]] auto handshakeTimeout() const noexcept -> time::TimeDelta { return _handshakeTimeout; }
    /// Set the TLS handshake timeout.
    /// @param value The positive timeout after TCP establishment.
    /// @return This options object for chaining.
    auto setHandshakeTimeout(const time::TimeDelta value) noexcept -> TlsClientConnectOptions & {
        _handshakeTimeout = value;
        return *this;
    }
    /// Get the authenticated application idle timeout.
    [[nodiscard]] auto idleTimeout() const noexcept -> time::TimeDelta { return _idleTimeout; }
    /// Set the authenticated application idle timeout.
    /// @param value The positive timeout between application activities.
    /// @return This options object for chaining.
    auto setIdleTimeout(const time::TimeDelta value) noexcept -> TlsClientConnectOptions & {
        _idleTimeout = value;
        return *this;
    }
    /// Get the graceful TLS closure timeout.
    [[nodiscard]] auto closeTimeout() const noexcept -> time::TimeDelta { return _closeTimeout; }
    /// Set the graceful TLS closure timeout.
    /// @param value The positive bidirectional close-notify timeout.
    /// @return This options object for chaining.
    auto setCloseTimeout(const time::TimeDelta value) noexcept -> TlsClientConnectOptions & {
        _closeTimeout = value;
        return *this;
    }

private:
    text::String _configurationLabel{cDefaultConfigurationLabel}; ///< Registry label resolved at startup.
    TcpConnectOptions _tcpOptions;                                ///< DNS and TCP options.
    SocketBufferLimits _bufferLimits;                             ///< TLS-owned queue limits.
    std::vector<text::String> _alpnProtocols;                     ///< Ordered ALPN offer.
    time::TimeDelta _handshakeTimeout{cDefaultHandshakeTimeout};  ///< TLS handshake deadline.
    time::TimeDelta _idleTimeout{cDefaultIdleTimeout};            ///< Authenticated application idle deadline.
    time::TimeDelta _closeTimeout{cDefaultCloseTimeout};          ///< Graceful close deadline.
};

}
