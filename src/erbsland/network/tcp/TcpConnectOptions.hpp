// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectOptions_fwd.hpp"

#include "../host_lookup/HostLookupOptions.hpp"
#include "../source/SocketBufferLimits.hpp"

#include "../../time/TimeDelta.hpp"

namespace erbsland::network {

/// Options captured when an outgoing TCP connection is started.
/// @tested{TcpConnectionTest}
class TcpConnectOptions final {
public:
    /// The default deadline for resolution and all connection attempts.
    inline static const auto cDefaultTimeout = time::TimeDelta::seconds(30);

public: // accessors
    /// Get the overall resolution-and-connection timeout.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _timeout; }
    /// Set the overall resolution-and-connection timeout.
    auto setTimeout(const time::TimeDelta value) noexcept -> TcpConnectOptions & {
        _timeout = value;
        return *this;
    }
    /// Get the host-lookup options.
    [[nodiscard]] auto hostLookupOptions() const noexcept -> HostLookupOptions { return _hostLookupOptions; }
    /// Set the host-lookup options.
    auto setHostLookupOptions(const HostLookupOptions value) noexcept -> TcpConnectOptions & {
        _hostLookupOptions = value;
        return *this;
    }
    /// Get the connected stream buffer limits.
    [[nodiscard]] constexpr auto bufferLimits() const noexcept -> SocketBufferLimits { return _bufferLimits; }
    /// Set the connected stream buffer limits.
    auto setBufferLimits(const SocketBufferLimits value) noexcept -> TcpConnectOptions & {
        _bufferLimits = value;
        return *this;
    }

private:
    time::TimeDelta _timeout{cDefaultTimeout}; ///< Overall resolution-and-connection timeout.
    HostLookupOptions _hostLookupOptions;      ///< Host-lookup policy.
    SocketBufferLimits _bufferLimits;          ///< Connected stream buffer limits.
};

}
