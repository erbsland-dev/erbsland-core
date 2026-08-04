// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NetworkErrorContext_fwd.hpp"
#include "NetworkErrorPhase.hpp"
#include "NetworkErrorReason.hpp"

#include "../Host.hpp"
#include "../HostEndpoint.hpp"
#include "../IpEndpoint.hpp"
#include "../tls/TlsAlertDescription.hpp"

#include "../../system/PlatformErrorContext_fwd.hpp"
#include "../../text/String.hpp"

#include <optional>

namespace erbsland::network {

/// Structured and machine-readable context for an asynchronous network failure.
/// @tested{HostLookupTest NetworkFacadeTest TlsClientConnectionTest}
class NetworkErrorContext final {
public:
    /// Create a network error context.
    /// @param title The short error title.
    /// @param description The user-facing error description.
    NetworkErrorContext(text::String title, text::String description) noexcept :
        _title{std::move(title)}, _description{std::move(description)} {}

public: // accessors
    /// Get the short error title.
    /// @return The title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _title; }
    /// Get the user-facing error description.
    /// @return The description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    /// Get the machine-readable error reason.
    [[nodiscard]] auto reason() const noexcept -> NetworkErrorReason { return _reason; }
    /// Get the operation phase in which the failure occurred.
    [[nodiscard]] auto phase() const noexcept -> NetworkErrorPhase { return _phase; }
    /// Get the public TLS alert associated with the failure.
    [[nodiscard]] auto tlsAlert() const noexcept -> const std::optional<TlsAlertDescription> & { return _tlsAlert; }
    /// Get the optional host involved in the operation.
    /// @return The host, or `std::nullopt` if it is unavailable.
    [[nodiscard]] auto host() const noexcept -> const std::optional<Host> & { return _host; }
    /// Get the optional local endpoint.
    /// @return The local endpoint, or `std::nullopt` if it is unavailable.
    [[nodiscard]] auto localEndpoint() const noexcept -> const std::optional<IpEndpoint> & { return _localEndpoint; }
    /// Get the optional remote endpoint.
    /// @return The remote endpoint, or `std::nullopt` if it is unavailable.
    [[nodiscard]] auto remoteEndpoint() const noexcept -> const std::optional<HostEndpoint> & {
        return _remoteEndpoint;
    }
    /// Get the optional platform error context.
    /// @return The platform context, or a null pointer if it is unavailable.
    [[nodiscard]] auto platformContext() const noexcept -> const system::PlatformErrorContextConstPtr & {
        return _platformContext;
    }
    /// Set the local endpoint.
    /// @param endpoint The local endpoint involved in the operation.
    /// @return This context for chaining.
    auto setLocalEndpoint(IpEndpoint endpoint) noexcept -> NetworkErrorContext & {
        _localEndpoint = std::move(endpoint);
        return *this;
    }
    /// Set the machine-readable error reason.
    /// @param reason The portable error reason.
    /// @return This context for chaining.
    auto setReason(const NetworkErrorReason reason) noexcept -> NetworkErrorContext & {
        _reason = reason;
        return *this;
    }
    /// Set the operation phase.
    /// @param phase The phase in which the failure occurred.
    /// @return This context for chaining.
    auto setPhase(const NetworkErrorPhase phase) noexcept -> NetworkErrorContext & {
        _phase = phase;
        return *this;
    }
    /// Set the associated TLS alert.
    /// @param alert The exact public TLS alert description.
    /// @return This context for chaining.
    auto setTlsAlert(const TlsAlertDescription alert) noexcept -> NetworkErrorContext & {
        _tlsAlert = alert;
        return *this;
    }
    /// Set the host involved in the operation.
    /// @param host The host involved in the operation.
    /// @return This context for chaining.
    auto setHost(Host host) noexcept -> NetworkErrorContext & {
        _host = std::move(host);
        return *this;
    }
    /// Set the remote endpoint.
    /// @param endpoint The remote endpoint involved in the operation.
    /// @return This context for chaining.
    auto setRemoteEndpoint(HostEndpoint endpoint) noexcept -> NetworkErrorContext & {
        _remoteEndpoint = std::move(endpoint);
        return *this;
    }
    /// Set the platform error context.
    /// @param context The platform-specific error details.
    /// @return This context for chaining.
    auto setPlatformContext(system::PlatformErrorContextConstPtr context) noexcept -> NetworkErrorContext & {
        _platformContext = std::move(context);
        return *this;
    }

private:
    text::String _title;                                     ///< The short error title.
    text::String _description;                               ///< The user-facing description.
    NetworkErrorReason _reason{NetworkErrorReason::Unknown}; ///< The machine-readable reason.
    NetworkErrorPhase _phase{NetworkErrorPhase::None};       ///< Operation phase.
    std::optional<TlsAlertDescription> _tlsAlert;            ///< Optional TLS alert.
    std::optional<Host> _host;                               ///< The optional host.
    std::optional<IpEndpoint> _localEndpoint;                ///< The optional local endpoint.
    std::optional<HostEndpoint> _remoteEndpoint;             ///< The optional remote endpoint.
    system::PlatformErrorContextConstPtr _platformContext;   ///< The optional platform details.
};

}
