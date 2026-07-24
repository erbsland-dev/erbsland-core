// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IpAddress.hpp"

#include "../text/FormatAs.hpp"

#include <compare>
#include <cstdint>
#include <optional>

namespace erbsland::network {

/// A normalized IPv4 or IPv6 CIDR network.
/// @tested{IpNetworkTest}
class IpNetwork final {
public:
    /// Create the IPv4 default route (`0.0.0.0/0`).
    IpNetwork() noexcept = default;
    /// Create and normalize a network.
    /// @param address An address within the network.
    /// @param prefixLength The number of fixed leading address bits.
    /// @throws err::ParameterError If the prefix length is invalid for the address version.
    IpNetwork(IpAddress address, uint8_t prefixLength);

public: // operators
    /// Compare two normalized networks.
    /// @param other The network to compare with this network.
    /// @return The strong ordering between the network values.
    [[nodiscard]] auto operator<=>(const IpNetwork &other) const noexcept -> std::strong_ordering = default;

public: // accessors
    /// Get the normalized first address.
    /// @return The first address in the network.
    [[nodiscard]] auto address() const noexcept -> const IpAddress & { return _address; }
    /// Get the prefix length.
    /// @return The number of fixed leading bits.
    [[nodiscard]] constexpr auto prefixLength() const noexcept -> uint8_t { return _prefixLength; }
    /// Get the first address in the network.
    /// @return The inclusive first address.
    [[nodiscard]] auto firstAddress() const noexcept -> IpAddress { return _address; }
    /// Get the last address in the network.
    /// @return The inclusive last address.
    [[nodiscard]] auto lastAddress() const noexcept -> IpAddress;

public: // tests
    /// Test if an address belongs to this network.
    /// @param address The address to test.
    /// @return `true` if the versions match and the address is within this network.
    [[nodiscard]] auto contains(const IpAddress &address) const noexcept -> bool;
    /// Test if another network is fully contained in this network.
    /// @param network The network to test.
    /// @return `true` if the complete network is within this network.
    [[nodiscard]] auto contains(const IpNetwork &network) const noexcept -> bool;

public: // conversion
    /// Format the normalized network in CIDR notation.
    /// @return The canonical address and decimal prefix length.
    [[nodiscard]] auto toString() const -> text::String;
    /// Calculate a hash value consistent with network equality.
    /// @return The hash value.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t;
    /// Parse and normalize an IPv4 or IPv6 CIDR network.
    /// @param text The complete CIDR text.
    /// @return The network, or `std::nullopt` if the text is invalid.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<IpNetwork>;
    /// Parse and normalize an IPv4 or IPv6 CIDR network.
    /// @param text The complete CIDR text.
    /// @return The parsed network.
    /// @throws err::ParseError If the text is not a valid CIDR network.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> IpNetwork;

private:
    [[nodiscard]] static auto maximumPrefix(IpVersion version) noexcept -> uint8_t;
    [[nodiscard]] static auto normalizedAddress(const IpAddress &address, uint8_t prefixLength) noexcept -> IpAddress;

private:
    IpAddress _address;      ///< The normalized first address of the network.
    uint8_t _prefixLength{}; ///< The number of fixed prefix bits.
};

}

template <>
struct std::hash<erbsland::network::IpNetwork> {
    auto operator()(const erbsland::network::IpNetwork &value) const noexcept -> std::size_t { return value.toHash(); }
};

template <>
struct erbsland::text::FormatAsText<erbsland::network::IpNetwork> : FormatAs<network::IpNetwork, String> {
    [[nodiscard]] auto format(const network::IpNetwork &value) const -> String { return value.toString(); }
};
