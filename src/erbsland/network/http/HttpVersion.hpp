// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <cstdint>

namespace erbsland::network {

/// A supported HTTP protocol version.
/// @seedoc{/reference/network/http_values}
/// @tested{HttpValueTest}
class HttpVersion final {
public:
    /// The wrapped protocol version.
    enum Value : uint8_t {
        Invalid = 0U, ///< No valid HTTP version.
        Http10 = 1U,  ///< HTTP/1.0.
        Http11 = 2U,  ///< HTTP/1.1.
    };

public:
    /// Create a protocol version from a value.
    constexpr HttpVersion(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr HttpVersion() noexcept = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const HttpVersion &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const HttpVersion &other, value, other._value);

public: // tests/accessors
    /// Test whether this version is supported.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _value != Invalid; }
    /// Get the raw version value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }
    /// Get the major version number, or zero for an invalid version.
    [[nodiscard]] constexpr auto major() const noexcept -> uint8_t { return isValid() ? 1U : 0U; }
    /// Get the minor version number, or zero for an invalid version.
    [[nodiscard]] constexpr auto minor() const noexcept -> uint8_t { return _value == Http11 ? 1U : 0U; }

public: // conversion
    /// Convert this version to an HTTP-version token.
    [[nodiscard]] auto toString() const noexcept -> text::String;
    /// Parse an exact HTTP-version token, returning an invalid placeholder on failure.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> HttpVersion;
    /// Parse an exact HTTP-version token.
    /// @throws err::ParseError If the version is malformed or unsupported.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> HttpVersion;

private:
    Value _value{Invalid}; ///< Wrapped protocol version.
};

}
