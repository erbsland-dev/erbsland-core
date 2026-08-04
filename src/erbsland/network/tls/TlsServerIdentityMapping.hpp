// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../HostName.hpp"

#include "../../text/String.hpp"

#include <utility>

namespace erbsland::network {

/// An exact canonical SNI host name mapped to an application TLS configuration label.
/// @tested{TlsServerConnectionTest}
class TlsServerIdentityMapping final {
public:
    /// Create one exact SNI mapping.
    /// @param serverName The exact canonical SNI name.
    /// @param configurationLabel The application TLS configuration label to resolve.
    TlsServerIdentityMapping(HostName serverName, text::String configurationLabel) :
        _serverName{std::move(serverName)}, _configurationLabel{std::move(configurationLabel)} {}

public:
    /// Get the exact canonical SNI host name.
    [[nodiscard]] auto serverName() const noexcept -> const HostName & { return _serverName; }
    /// Get the TLS configuration label resolved before accepting TCP.
    [[nodiscard]] auto configurationLabel() const noexcept -> const text::String & { return _configurationLabel; }

private:
    HostName _serverName;             ///< Exact canonical SNI name.
    text::String _configurationLabel; ///< Requested application configuration label.
};

}
