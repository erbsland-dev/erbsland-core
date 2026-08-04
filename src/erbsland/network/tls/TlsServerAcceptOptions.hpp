// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsServerAcceptOptions_fwd.hpp"
#include "TlsServerIdentityMapping.hpp"

#include "../source/ConnectionQuota.hpp"
#include "../source/SocketBufferLimits.hpp"
#include "../tcp/TcpAcceptOptions.hpp"

#include "../../cryptology/tls/TlsCipherSuite.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/String.hpp"
#include "../../time/TimeDelta.hpp"

#include <utility>
#include <vector>

namespace erbsland::network {

/// Options captured when an incoming TLS server connection is accepted.
/// @tested{TlsServerConnectionTest}
class TlsServerAcceptOptions final {
public:
    /// Reserved framework label for regular TLS servers.
    inline static const auto cDefaultConfigurationLabel = text::String{"tls/server"};
    /// Default handshake deadline after TCP adoption.
    inline static const auto cDefaultHandshakeTimeout = time::TimeDelta::seconds(30);
    /// Default authenticated application idle deadline.
    inline static const auto cDefaultIdleTimeout = time::TimeDelta::minutes(5);
    /// Default bidirectional close-notify deadline.
    inline static const auto cDefaultCloseTimeout = time::TimeDelta::seconds(10);
    /// Maximum accepted TLS-owned queue limit in either direction.
    static constexpr auto cMaximumBufferLength = unit::ByteLength{16U * 1024U * 1024U};
    /// Maximum one-shot application send size.
    static constexpr auto cMaximumApplicationSendLength = unit::ByteLength{16U * 1024U};
    /// Maximum number of exact SNI identity mappings.
    static constexpr auto cMaximumIdentityMappings = unit::ItemCount{64U};

public:
    /// Create options using a required shared handshake quota.
    /// @param handshakeQuota The quota held until peer Finished is authenticated.
    explicit TlsServerAcceptOptions(ConnectionQuotaPtr handshakeQuota) : _handshakeQuota{std::move(handshakeQuota)} {
        const auto suites = cryptology::TlsCipherSuite::all();
        _cipherSuites.assign(suites.begin(), suites.end());
    }

public: // accessors
    /// Get the shared incomplete-handshake quota.
    [[nodiscard]] auto handshakeQuota() const noexcept -> const ConnectionQuotaPtr & { return _handshakeQuota; }
    /// Get the required default identity configuration label.
    [[nodiscard]] auto configurationLabel() const noexcept -> const text::String & { return _configurationLabel; }
    /// Set the required default identity configuration label.
    /// @param value The exact or descendant registry label to resolve.
    /// @return This options object for chaining.
    auto setConfigurationLabel(text::String value) noexcept -> TlsServerAcceptOptions & {
        _configurationLabel = std::move(value);
        return *this;
    }
    /// Get the exact canonical SNI identity mappings.
    [[nodiscard]] auto identityMappings() const noexcept -> const std::vector<TlsServerIdentityMapping> & {
        return _identityMappings;
    }
    /// Replace all exact canonical SNI identity mappings.
    /// @param value At most 64 mappings with unique server names.
    /// @return This options object for chaining.
    auto setIdentityMappings(std::vector<TlsServerIdentityMapping> value) noexcept -> TlsServerAcceptOptions & {
        _identityMappings = std::move(value);
        return *this;
    }
    /// Get the nested accepted TCP stream options.
    [[nodiscard]] auto tcpOptions() const noexcept -> const TcpAcceptOptions & { return _tcpOptions; }
    /// Set the nested accepted TCP stream options.
    /// @param value The accepted TCP stream options.
    /// @return This options object for chaining.
    auto setTcpOptions(TcpAcceptOptions value) noexcept -> TlsServerAcceptOptions & {
        _tcpOptions = std::move(value);
        return *this;
    }
    /// Get the TLS protocol queue limits.
    [[nodiscard]] auto bufferLimits() const noexcept -> SocketBufferLimits { return _bufferLimits; }
    /// Set the TLS protocol queue limits.
    /// @param value The TLS-owned send and aggregate receive bounds.
    /// @return This options object for chaining.
    auto setBufferLimits(const SocketBufferLimits value) noexcept -> TlsServerAcceptOptions & {
        _bufferLimits = value;
        return *this;
    }
    /// Get supported opaque ALPN identifiers in server-preference order.
    [[nodiscard]] auto alpnProtocols() const noexcept -> const std::vector<mem::ByteBlock> & { return _alpnProtocols; }
    /// Set supported opaque ALPN identifiers in server-preference order.
    /// @param value The bounded non-empty protocol identifiers.
    /// @return This options object for chaining.
    auto setAlpnProtocols(std::vector<mem::ByteBlock> value) noexcept -> TlsServerAcceptOptions & {
        _alpnProtocols = std::move(value);
        return *this;
    }
    /// Get enabled cipher suites in server-preference order.
    [[nodiscard]] auto cipherSuites() const noexcept -> const std::vector<cryptology::TlsCipherSuite> & {
        return _cipherSuites;
    }
    /// Set enabled cipher suites in server-preference order.
    /// @param value The non-empty unique suite list.
    /// @return This options object for chaining.
    auto setCipherSuites(std::vector<cryptology::TlsCipherSuite> value) noexcept -> TlsServerAcceptOptions & {
        _cipherSuites = std::move(value);
        return *this;
    }
    /// Get the TLS handshake timeout.
    [[nodiscard]] auto handshakeTimeout() const noexcept -> time::TimeDelta { return _handshakeTimeout; }
    /// Set the TLS handshake timeout.
    /// @param value The positive timeout after TCP adoption.
    /// @return This options object for chaining.
    auto setHandshakeTimeout(const time::TimeDelta value) noexcept -> TlsServerAcceptOptions & {
        _handshakeTimeout = value;
        return *this;
    }
    /// Get the authenticated application idle timeout.
    [[nodiscard]] auto idleTimeout() const noexcept -> time::TimeDelta { return _idleTimeout; }
    /// Set the authenticated application idle timeout.
    /// @param value The positive timeout between application activities.
    /// @return This options object for chaining.
    auto setIdleTimeout(const time::TimeDelta value) noexcept -> TlsServerAcceptOptions & {
        _idleTimeout = value;
        return *this;
    }
    /// Get the graceful TLS closure timeout.
    [[nodiscard]] auto closeTimeout() const noexcept -> time::TimeDelta { return _closeTimeout; }
    /// Set the graceful TLS closure timeout.
    /// @param value The positive bidirectional close-notify timeout.
    /// @return This options object for chaining.
    auto setCloseTimeout(const time::TimeDelta value) noexcept -> TlsServerAcceptOptions & {
        _closeTimeout = value;
        return *this;
    }

private:
    ConnectionQuotaPtr _handshakeQuota;                           ///< Shared incomplete-handshake quota.
    text::String _configurationLabel{cDefaultConfigurationLabel}; ///< Required default identity label.
    std::vector<TlsServerIdentityMapping> _identityMappings;      ///< Exact SNI label mappings.
    TcpAcceptOptions _tcpOptions;                                 ///< Accepted TCP stream options.
    SocketBufferLimits _bufferLimits;                             ///< TLS queue limits.
    std::vector<mem::ByteBlock> _alpnProtocols;                   ///< Server-preference ALPN identifiers.
    std::vector<cryptology::TlsCipherSuite> _cipherSuites;        ///< Server-preference cipher suites.
    time::TimeDelta _handshakeTimeout{cDefaultHandshakeTimeout};  ///< Handshake deadline.
    time::TimeDelta _idleTimeout{cDefaultIdleTimeout};            ///< Application idle deadline.
    time::TimeDelta _closeTimeout{cDefaultCloseTimeout};          ///< Close-notify deadline.
};

}
