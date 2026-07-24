// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostName.hpp"
#include "IpAddress.hpp"

#include <cstddef>
#include <optional>
#include <variant>

namespace erbsland::network {

/// A numeric IP address or unresolved host name.
/// @tested{NetworkValueTest}
class Host final {
public:
    /// Create the IPv4 any address as host.
    Host() noexcept = default;
    /// Create a host from an IP address.
    /// @param address The resolved IP address.
    Host(IpAddress address) noexcept : _value{std::move(address)} {} // NOLINT(*-explicit-constructor)
    /// Create a host from an unresolved name.
    /// @param name The platform-resolvable host name.
    Host(HostName name) noexcept : _value{std::move(name)} {} // NOLINT(*-explicit-constructor)

public:                                                       // operators
    /// Compare two hosts.
    /// @param other The host to compare with this host.
    /// @return `true` if both hosts contain the same alternative and value.
    [[nodiscard]] auto operator==(const Host &other) const noexcept -> bool = default;

public: // tests/accessors
    /// Test if this host contains an IP address.
    /// @return `true` if this is a resolved address.
    [[nodiscard]] auto isAddress() const noexcept -> bool { return std::holds_alternative<IpAddress>(_value); }
    /// Test if this host contains an unresolved name.
    /// @return `true` if this is a host name.
    [[nodiscard]] auto isName() const noexcept -> bool { return std::holds_alternative<HostName>(_value); }
    /// Get the contained address.
    /// @return The address, or `std::nullopt` if this host contains a name.
    [[nodiscard]] auto address() const noexcept -> std::optional<IpAddress>;
    /// Get the contained host name.
    /// @return The name, or `std::nullopt` if this host contains an address.
    [[nodiscard]] auto name() const noexcept -> std::optional<HostName>;

public: // conversion
    /// Format the contained address or host name.
    /// @return The host text.
    [[nodiscard]] auto toString() const -> text::String;
    /// Calculate a hash value consistent with host equality.
    /// @return The hash value.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t;
    /// Parse an IP address or platform-resolvable host name.
    /// @param text The complete host text without a port.
    /// @return The parsed host, or `std::nullopt` if the text is invalid.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<Host>;
    /// Parse an IP address or platform-resolvable host name.
    /// @param text The complete host text without a port.
    /// @return The parsed host.
    /// @throws err::ParseError If the text is not a valid host.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> Host;

private:
    std::variant<IpAddress, HostName> _value; ///< The resolved address or unresolved name.
};

}

template <>
struct std::hash<erbsland::network::Host> {
    auto operator()(const erbsland::network::Host &value) const noexcept -> std::size_t { return value.toHash(); }
};
