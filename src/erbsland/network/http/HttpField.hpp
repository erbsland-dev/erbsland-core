// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpFieldName.hpp"

#include "../../text/String.hpp"

#include <compare>
#include <utility>

namespace erbsland::network {

/// One validated HTTP field with an exact value.
/// Field values may contain obs-text and malformed UTF-8 bytes, but never prohibited HTTP control bytes.
/// @seedoc{/reference/network/http_protocol}
/// @tested{HttpHeadersTest}
class HttpField final {
public:
    /// Create an invalid empty placeholder.
    HttpField() noexcept = default;
    /// Create a field from a validated name and exact value.
    /// @throws err::ParameterError If the name is invalid or the value contains a prohibited control byte.
    HttpField(HttpFieldName name, text::String value);
    /// Create a field from name text and an exact value.
    /// @throws err::ParameterError If the name or value is invalid.
    HttpField(text::String name, text::String value);

    // defaults
    ~HttpField() = default;
    HttpField(const HttpField &) noexcept = default;
    HttpField(HttpField &&) noexcept = default;
    auto operator=(const HttpField &) noexcept -> HttpField & = default;
    auto operator=(HttpField &&) noexcept -> HttpField & = default;

public: // operators
    /// Compare the case-insensitive name and exact value.
    [[nodiscard]] auto operator<=>(const HttpField &other) const noexcept -> std::strong_ordering = default;

public: // tests/accessors
    /// Test whether this field has a valid name.
    [[nodiscard]] auto isValid() const noexcept -> bool { return _name.isValid(); }
    /// Get the validated field name.
    [[nodiscard]] auto name() const noexcept -> const HttpFieldName & { return _name; }
    /// Get the exact field value.
    [[nodiscard]] auto value() const noexcept -> const text::String & { return _value; }

private:
    HttpFieldName _name; ///< Validated field name.
    text::String _value; ///< Exact field value bytes.
};

}
