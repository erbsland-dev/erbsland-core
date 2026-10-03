// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpMediaTypeParameter.hpp"

#include "../../text/StringEditor_fwd.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../util/List.hpp"

#include <compare>
#include <optional>
#include <utility>

namespace erbsland::network {

/// An ordered copy-on-write list of media-type parameters.
using HttpMediaTypeParameters = util::List<HttpMediaTypeParameter>;

/// A validated HTTP media type with ordered unique parameters.
/// Type, subtype, and parameter names are canonical lowercase ASCII. Parameter values retain their semantic text.
/// @seedoc{/reference/network/http_protocol}
/// @tested{HttpValueTest}
class HttpMediaType final {
public:
    /// Maximum standalone media-type text length.
    static constexpr auto cMaximumTextLength = unit::ByteCount{16U * 1024U};
    /// Maximum standalone parameter count.
    static constexpr auto cMaximumParameterCount = unit::ItemCount{64U};

public:
    /// Create an invalid placeholder.
    HttpMediaType() noexcept = default;
    /// Create a media type without parameters.
    /// @param type The media type. Must be a non-empty ASCII token.
    /// @param subtype The media subtype. Must be a non-empty ASCII token.
    HttpMediaType(text::String type, text::String subtype);
    /// Create a media type with ordered unique parameters.
    /// @param type The media type. Must be a non-empty ASCII token.
    /// @param subtype The media subtype. Must be a non-empty ASCII token.
    /// @param parameters The ordered media-type parameters. Names must be unique.
    HttpMediaType(text::String type, text::String subtype, HttpMediaTypeParameters parameters);

    // defaults
    ~HttpMediaType() = default;
    HttpMediaType(const HttpMediaType &) noexcept = default;
    HttpMediaType(HttpMediaType &&) noexcept = default;
    auto operator=(const HttpMediaType &) noexcept -> HttpMediaType & = default;
    auto operator=(HttpMediaType &&) noexcept -> HttpMediaType & = default;

public: // operators
    /// Compare canonical type, subtype, and ordered parameters.
    [[nodiscard]] auto operator<=>(const HttpMediaType &other) const noexcept -> std::strong_ordering = default;

public: // tests/accessors
    /// Test whether this media type is valid.
    [[nodiscard]] auto isValid() const noexcept -> bool { return !_type.isEmpty(); }
    /// Get the canonical lowercase top-level type.
    [[nodiscard]] auto type() const noexcept -> const text::String & { return _type; }
    /// Get the canonical lowercase subtype.
    [[nodiscard]] auto subtype() const noexcept -> const text::String & { return _subtype; }
    /// Get a copy-on-write copy of the ordered parameters.
    [[nodiscard]] auto parameters() const noexcept -> HttpMediaTypeParameters { return _parameters; }
    /// Test whether a parameter is present using ASCII-case-insensitive name matching.
    [[nodiscard]] auto hasParameter(const text::String &name) const noexcept -> bool;
    /// Get the first matching semantic parameter value.
    [[nodiscard]] auto parameter(const text::String &name) const noexcept -> std::optional<text::String>;

public: // modifiers
    /// Set a parameter in place, preserving the first matching position or appending it.
    /// @param parameter The validated media-type parameter. Must be valid.
    auto setParameter(HttpMediaTypeParameter parameter) -> HttpMediaType &;
    /// Set a parameter in place from text.
    /// @param name The parameter name. Must be a non-empty ASCII token.
    /// @param value The parameter value. Must be valid UTF-8 without prohibited HTTP control bytes.
    auto setParameter(text::String name, text::String value) -> HttpMediaType &;
    /// Remove a parameter using ASCII-case-insensitive name matching.
    auto removeParameter(const text::String &name) -> HttpMediaType &;

public: // conversion
    /// Format this media type canonically.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse a media type, returning an invalid placeholder on failure.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> HttpMediaType;
    /// Parse a media type.
    /// @throws err::ParseError If the syntax, limits, or parameter uniqueness rules are violated.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> HttpMediaType;

private:
    /// Validate and canonicalize the primary type tokens and parameter list.
    void initialize(text::String type, text::String subtype, HttpMediaTypeParameters parameters);
    /// Test whether the parameter list has unique names.
    [[nodiscard]] static auto hasUniqueNames(const HttpMediaTypeParameters &parameters) noexcept -> bool;
    /// Write one parameter value using token or quoted-string syntax.
    static void writeParameterValue(text::StringEditor &editor, const text::String &value);

private:
    text::String _type;                  ///< Canonical lowercase top-level type.
    text::String _subtype;               ///< Canonical lowercase subtype.
    HttpMediaTypeParameters _parameters; ///< Ordered unique parameters.
};

}
