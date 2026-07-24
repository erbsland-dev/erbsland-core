// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IpAddress.hpp"
#include "Port.hpp"
#include "ScopeId.hpp"

#include <optional>

namespace erbsland::network {

/// A resolved IP endpoint.
/// @tested{NetworkValueTest}
class IpEndpoint final {
public:
    /// Create the IPv4 any address with the automatic port.
    IpEndpoint() noexcept = default;
    /// Create a resolved endpoint.
    /// @param address The resolved IP address.
    /// @param port The transport port.
    /// @param scopeId The numeric IPv6 scope identifier, or zero if unspecified.
    IpEndpoint(IpAddress address, Port port, ScopeId scopeId = {}) noexcept :
        _address{std::move(address)}, _port{port}, _scopeId{scopeId} {}

public: // operators
    /// Compare two resolved endpoints.
    /// @param other The endpoint to compare with this endpoint.
    /// @return The strong ordering between the endpoint values.
    [[nodiscard]] auto operator<=>(const IpEndpoint &other) const noexcept -> std::strong_ordering = default;

public: // accessors
    /// Get the resolved IP address.
    /// @return The endpoint address.
    [[nodiscard]] auto address() const noexcept -> const IpAddress & { return _address; }
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
    /// Parse a resolved endpoint.
    /// @param text The complete address, optional IPv6 scope, and port text.
    /// @return The endpoint, or `std::nullopt` if the text is invalid.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<IpEndpoint>;
    /// Parse a resolved endpoint.
    /// @param text The complete address, optional IPv6 scope, and port text.
    /// @return The parsed endpoint.
    /// @throws err::ParseError If the text is not a valid resolved endpoint.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> IpEndpoint;

private:
    IpAddress _address; ///< The resolved IP address.
    Port _port;         ///< The transport port.
    ScopeId _scopeId;   ///< The optional numeric IPv6 scope identifier.
};

}

template <>
struct std::hash<erbsland::network::IpEndpoint> {
    auto operator()(const erbsland::network::IpEndpoint &value) const noexcept -> std::size_t { return value.toHash(); }
};
