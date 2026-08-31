// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../text/StringList_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::log {

/// The transport used for standards-based syslog output.
/// @tested{LogCoreTest SyslogLogWriterTest}
class SyslogTransport final {
public:
    /// The raw transport value.
    enum Value : uint8_t {
        Udp, ///< RFC 5424 messages in UDP datagrams.
        Tcp, ///< RFC 5424 messages with RFC 6587 octet-counted framing.
        Tls, ///< RFC 5424 messages with RFC 6587 framing over TLS.
    };

    /// Create the default UDP transport.
    constexpr SyslogTransport() noexcept = default;
    /// Create a transport from its raw value.
    /// @param value The raw transport value.
    constexpr SyslogTransport(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:                                                                      // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const SyslogTransport &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const SyslogTransport &other, value, other._value);

public: // accessors
    /// Get the raw transport value.
    /// @return The embedded value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this transport to its canonical configuration identifier.
    /// @return `udp`, `tcp`, or `tls`.
    [[nodiscard]] auto toString() const -> text::String;
    /// Get all canonical configuration identifiers.
    /// @return The identifiers accepted by `fromString()`, irrespective of ASCII letter case.
    [[nodiscard]] static auto allStrings() -> text::StringList;
    /// Parse a canonical transport identifier case-insensitively using ASCII folding.
    /// @param text The identifier to parse.
    /// @return The parsed transport, or an empty optional if the identifier is unknown.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<SyslogTransport>;
    /// Parse a canonical transport identifier.
    /// @param text The identifier to parse.
    /// @return The parsed transport.
    /// @throws err::ParseError If the identifier is unknown.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> SyslogTransport;

private:
    Value _value{Udp}; ///< Raw transport value.
};

}
