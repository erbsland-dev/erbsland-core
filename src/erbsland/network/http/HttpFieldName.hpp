// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpFieldType.hpp"

#include "../../text/String.hpp"

#include <compare>
#include <cstddef>
#include <utility>

namespace erbsland::network {

/// A validated case-insensitive HTTP field name.
/// The original ASCII spelling is retained while equality, ordering, and hashing use ASCII case folding.
/// @seedoc{/reference/network/http_protocol}
/// @tested{HttpHeadersTest}
class HttpFieldName final {
public:
    /// Create an invalid placeholder.
    HttpFieldName() noexcept = default;
    /// Create the conventional name for a recognized field type.
    HttpFieldName(HttpFieldType type) noexcept; // NOLINT(*-explicit-constructor)
    /// Create a validated field name.
    /// @throws err::ParameterError If the text is not a non-empty ASCII token.
    explicit HttpFieldName(text::String text);

    // defaults
    ~HttpFieldName() = default;
    HttpFieldName(const HttpFieldName &) noexcept = default;
    HttpFieldName(HttpFieldName &&) noexcept = default;
    auto operator=(const HttpFieldName &) noexcept -> HttpFieldName & = default;
    auto operator=(HttpFieldName &&) noexcept -> HttpFieldName & = default;

public: // operators
    /// Compare field names with ASCII case folding.
    [[nodiscard]] auto operator<=>(const HttpFieldName &other) const noexcept -> std::strong_ordering;
    /// Test field-name equality with ASCII case folding.
    [[nodiscard]] auto operator==(const HttpFieldName &other) const noexcept -> bool;

public: // tests/accessors
    /// Test whether this name is valid.
    [[nodiscard]] auto isValid() const noexcept -> bool { return !_text.isEmpty(); }
    /// Get the retained field-name spelling.
    [[nodiscard]] auto text() const noexcept -> const text::String & { return _text; }
    /// Get the recognized field classification.
    [[nodiscard]] auto type() const noexcept -> HttpFieldType { return _type; }
    /// Create an ASCII-case-insensitive hash.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t { return _text.toHashCI(); }

public: // conversion
    /// Convert this name to its retained spelling.
    [[nodiscard]] auto toString() const noexcept -> text::String { return _text; }
    /// Parse a field name, returning an invalid placeholder on failure.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> HttpFieldName;
    /// Parse a field name.
    /// @throws err::ParseError If the text is not a non-empty ASCII token.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> HttpFieldName;

private:
    /// Create a validated name without another validation pass.
    HttpFieldName(text::String text, HttpFieldType type) noexcept : _text{std::move(text)}, _type{type} {}

private:
    text::String _text;  ///< Retained valid field-name spelling.
    HttpFieldType _type; ///< Recognized field classification.
};

}

namespace std {

template <>
struct hash<erbsland::network::HttpFieldName> {
    auto operator()(const erbsland::network::HttpFieldName &value) const noexcept -> std::size_t {
        return value.toHash();
    }
};

}
