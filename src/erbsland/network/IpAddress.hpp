// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IpVersion.hpp"

#include "../mem/ByteArray.hpp"
#include "../text/FormatAs.hpp"
#include "../text/String.hpp"
#include "../text/StringCharReader_fwd.hpp"

#include <compare>
#include <cstddef>
#include <functional>
#include <optional>

namespace erbsland::network {

/// An IPv4 or IPv6 address.
/// @tested{IpAddressTest}
class IpAddress final {
public:
    /// The fixed 16-byte address storage in network byte order.
    using Bytes = mem::ByteArray<16>;

public:
    /// Create the IPv4 any address.
    constexpr IpAddress() noexcept = default;

    // defaults
    ~IpAddress() = default;
    IpAddress(const IpAddress &) noexcept = default;
    IpAddress(IpAddress &&) noexcept = default;
    auto operator=(const IpAddress &) noexcept -> IpAddress & = default;
    auto operator=(IpAddress &&) noexcept -> IpAddress & = default;

public: // operators
    /// Compare two addresses by version and binary value.
    /// @param other The address to compare with this address.
    /// @return The strong ordering between the addresses.
    [[nodiscard]] auto operator<=>(const IpAddress &other) const noexcept -> std::strong_ordering = default;

public: // tests
    /// Test if this is an IPv4 address.
    /// @return `true` for an IPv4 address.
    [[nodiscard]] constexpr auto isV4() const noexcept -> bool { return _version == IpVersion::V4; }
    /// Test if this is an IPv6 address.
    /// @return `true` for an IPv6 address.
    [[nodiscard]] constexpr auto isV6() const noexcept -> bool { return _version == IpVersion::V6; }
    /// Test if all address bits are zero.
    /// @return `true` for an IPv4 or IPv6 any address.
    [[nodiscard]] auto isAny() const noexcept -> bool;
    /// Test if this is an IPv4 or IPv6 loopback address.
    /// @return `true` for an address in `127.0.0.0/8` or the IPv6 address `::1`.
    [[nodiscard]] auto isLoopback() const noexcept -> bool;

public: // accessors
    /// Get the IP version.
    /// @return The address version.
    [[nodiscard]] constexpr auto version() const noexcept -> IpVersion { return _version; }
    /// Get the address bytes in network byte order.
    /// For IPv4 addresses, the first four bytes contain the address and the remaining bytes are zero.
    /// @return The fixed 16-byte address storage.
    [[nodiscard]] constexpr auto bytes() const noexcept -> const Bytes & { return _bytes; }

public: // conversion
    /// Format the address in canonical text form.
    /// @return The dotted-decimal IPv4 or lowercase compressed IPv6 representation.
    [[nodiscard]] auto toString() const -> text::String;
    /// Calculate a hash value consistent with address equality.
    /// @return The hash value.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t;
    /// Parse an IPv4 or IPv6 address.
    /// @param text The complete address text without a port or scope identifier.
    /// @return The parsed address, or `std::nullopt` if the text is invalid.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<IpAddress>;
    /// Parse an IPv4 or IPv6 address.
    /// @param text The complete address text without a port or scope identifier.
    /// @return The parsed address.
    /// @throws err::ParseError If the text is not a valid IPv4 or IPv6 address.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> IpAddress;
    /// Create an address from bytes in network byte order.
    /// For IPv4, bytes after the first four are cleared.
    /// @param version The IP version that determines how many bytes are significant.
    /// @param bytes The address bytes in network byte order.
    /// @return The created address.
    [[nodiscard]] static auto fromBytes(IpVersion version, Bytes bytes) noexcept -> IpAddress;

public: // predefined
    /// Create the IPv4 any address.
    /// @return The address `0.0.0.0`.
    [[nodiscard]] static auto anyV4() noexcept -> IpAddress;
    /// Create the IPv6 any address.
    /// @return The address `::`.
    [[nodiscard]] static auto anyV6() noexcept -> IpAddress;
    /// Create the canonical IPv4 loopback address.
    /// @return The address `127.0.0.1`.
    [[nodiscard]] static auto loopbackV4() noexcept -> IpAddress;
    /// Create the IPv6 loopback address.
    /// @return The address `::1`.
    [[nodiscard]] static auto loopbackV6() noexcept -> IpAddress;

private:
    using V4Bytes = mem::ByteArray<4>;

private:
    [[nodiscard]] static auto parseV4(text::StringCharReader &reader, V4Bytes &bytes) noexcept -> bool;
    [[nodiscard]] static auto parseV6(text::StringCharReader &reader, Bytes &bytes) noexcept -> bool;
    [[nodiscard]] static auto formatV4(const Bytes &bytes, std::size_t offset = 0U) -> text::String;
    [[nodiscard]] static auto isV4Mapped(const Bytes &bytes) noexcept -> bool;
    [[nodiscard]] static auto formatV6(const Bytes &bytes) -> text::String;
    constexpr IpAddress(IpVersion version, Bytes bytes) noexcept : _bytes{bytes}, _version{version} {}

private:
    Bytes _bytes{};                    ///< The address bytes in network byte order.
    IpVersion _version{IpVersion::V4}; ///< The address version.
};

}

template <>
struct std::hash<erbsland::network::IpAddress> {
    auto operator()(const erbsland::network::IpAddress &value) const noexcept -> std::size_t { return value.toHash(); }
};

template <>
struct erbsland::text::FormatAsText<erbsland::network::IpAddress> : FormatAs<network::IpAddress, String> {
    [[nodiscard]] auto format(const network::IpAddress &value) const -> String { return value.toString(); }
};
