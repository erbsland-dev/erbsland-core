// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"

#include <compare>
#include <utility>

namespace erbsland::network {

/// One validated HTTP media-type parameter.
/// Parameter names are canonical lowercase ASCII tokens. Values are semantic unquoted UTF-8 text.
/// @seedoc{/reference/network/http_values}
/// @tested{HttpValueTest}
class HttpMediaTypeParameter final {
public:
    /// Create a validated media-type parameter.
    /// @throws err::ParameterError If the name or value is invalid.
    HttpMediaTypeParameter(text::String name, text::String value);

    // defaults
    HttpMediaTypeParameter() noexcept = default;
    ~HttpMediaTypeParameter() = default;
    HttpMediaTypeParameter(const HttpMediaTypeParameter &) noexcept = default;
    HttpMediaTypeParameter(HttpMediaTypeParameter &&) noexcept = default;
    auto operator=(const HttpMediaTypeParameter &) noexcept -> HttpMediaTypeParameter & = default;
    auto operator=(HttpMediaTypeParameter &&) noexcept -> HttpMediaTypeParameter & = default;

public: // operators
    /// Compare canonical names and exact semantic values.
    [[nodiscard]] auto operator<=>(const HttpMediaTypeParameter &other) const noexcept
        -> std::strong_ordering = default;

public: // accessors
    /// Get the canonical lowercase parameter name.
    [[nodiscard]] auto name() const noexcept -> const text::String & { return _name; }
    /// Get the semantic parameter value.
    [[nodiscard]] auto value() const noexcept -> const text::String & { return _value; }

private:
    text::String _name;  ///< Canonical lowercase parameter name.
    text::String _value; ///< Semantic unquoted parameter value.
};

}
