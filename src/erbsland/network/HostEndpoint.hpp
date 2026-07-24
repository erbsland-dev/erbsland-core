// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Host.hpp"
#include "Port.hpp"
#include "ScopeId.hpp"

#include <optional>

namespace erbsland::network {

/// A host name or IP address paired with a network port.
/// @tested{NetworkValueTest}
class HostEndpoint final {
public:
    /// Create the IPv4 any address with the automatic port.
    HostEndpoint() noexcept = default;
    /// Create an endpoint from a resolved address or unresolved host name.
    /// @param host The resolved address or unresolved name.
    /// @param port The transport port.
    /// @param scopeId The numeric IPv6 scope identifier, or zero if unspecified.
    HostEndpoint(Host host, Port port, ScopeId scopeId = {}) noexcept :
        _host{std::move(host)}, _port{port}, _scopeId{scopeId} {}

public: // operators
    /// Compare two host endpoints.
    /// @param other The endpoint to compare with this endpoint.
    /// @return `true` if all endpoint components are equal.
    [[nodiscard]] auto operator==(const HostEndpoint &other) const noexcept -> bool = default;

public: // accessors
    /// Get the resolved address or unresolved host name.
    /// @return The endpoint host.
    [[nodiscard]] auto host() const noexcept -> const Host & { return _host; }
    /// Get the transport port.
    /// @return The endpoint port.
    [[nodiscard]] constexpr auto port() const noexcept -> Port { return _port; }
    /// Get the IPv6 scope identifier.
    /// @return The scope identifier, or zero if unspecified.
    [[nodiscard]] constexpr auto scopeId() const noexcept -> ScopeId { return _scopeId; }

public: // conversion
    /// Format the endpoint, including IPv6 brackets and an optional scope.
    /// @return The endpoint text.
    [[nodiscard]] auto toString() const -> text::String;
    /// Calculate a hash value consistent with endpoint equality.
    /// @return The hash value.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t;
    /// Parse a resolved or unresolved host endpoint.
    /// @param text The complete host, optional IPv6 scope, and port text.
    /// @return The endpoint, or `std::nullopt` if the text is invalid.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<HostEndpoint>;
    /// Parse a resolved or unresolved host endpoint.
    /// @param text The complete host, optional IPv6 scope, and port text.
    /// @return The parsed endpoint.
    /// @throws err::ParseError If the text is not a valid host endpoint.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> HostEndpoint;

private:
    Host _host;       ///< The resolved address or unresolved host name.
    Port _port;       ///< The transport port.
    ScopeId _scopeId; ///< The optional numeric IPv6 scope identifier.
};

}

template <>
struct std::hash<erbsland::network::HostEndpoint> {
    auto operator()(const erbsland::network::HostEndpoint &value) const noexcept -> std::size_t {
        return value.toHash();
    }
};
