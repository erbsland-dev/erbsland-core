// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsClientConfiguration.hpp"
#include "TlsConfiguration_fwd.hpp"
#include "TlsServerConfiguration.hpp"

#include "../../err/ParameterFailure.hpp"

namespace erbsland::cryptology {

/// Independent client and server parts sharing one application profile label.
/// @tested{CryptologyConfigurationTest TlsConfigurationParserTest}
class TlsConfiguration final {
public:
    /// Create a profile without either role.
    TlsConfiguration() = default;
    /// Create a client-only profile.
    /// @param client Complete client configuration.
    explicit TlsConfiguration(TlsClientConfiguration client) : _client{std::move(client)} {}
    /// Create a server-only profile.
    /// @param server Complete server configuration.
    explicit TlsConfiguration(TlsServerConfiguration server) : _server{std::move(server)} {}

public: // accessors
    /// Get the optional client part.
    [[nodiscard]] auto client() const noexcept -> const std::optional<TlsClientConfiguration> & { return _client; }
    /// Replace the client part without modifying the server part.
    void setClient(TlsClientConfiguration value) { _client = std::move(value); }
    /// Get the optional server part.
    [[nodiscard]] auto server() const noexcept -> const std::optional<TlsServerConfiguration> & { return _server; }
    /// Replace the server part without modifying the client part.
    void setServer(TlsServerConfiguration value) { _server = std::move(value); }
    /// Test whether this profile contains the requested local role.
    /// @param role A valid local TLS role.
    /// @return Whether that role is present.
    [[nodiscard]] auto hasRole(TlsRole role) const -> bool {
        validateRole(role);
        if (role == TlsRole::Client) {
            return _client.has_value();
        }
        return _server.has_value();
    }
    /// Remove one role without modifying the other.
    /// @param role A valid local TLS role.
    void clearRole(TlsRole role) {
        validateRole(role);
        if (role == TlsRole::Client) {
            _client.reset();
        } else {
            _server.reset();
        }
    }
    /// Replace only the role parts supplied by another profile.
    /// @param value The supplied replacement parts.
    void merge(const TlsConfiguration &value) {
        if (value._client) {
            _client = value._client;
        }
        if (value._server) {
            _server = value._server;
        }
    }
    /// Validate a local role.
    /// @param role The role to check.
    static void validateRole(TlsRole role) {
        using namespace text::literals;
        if (role != TlsRole::Client && role != TlsRole::Server) {
            throw err::ParameterFailure{"A valid TLS role is required."_el, "role"_el};
        }
    }

private:
    std::optional<TlsClientConfiguration> _client; ///< Optional independent client part.
    std::optional<TlsServerConfiguration> _server; ///< Optional independent server part.
};

}
