// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpMethodType.hpp"

#include "../../text/String.hpp"

#include <compare>
#include <cstddef>
#include <optional>
#include <utility>

namespace erbsland::network {

/// A validated standard or extension HTTP method.
/// Method names are case-sensitive. Standard classification recognizes only canonical uppercase spellings.
/// @seedoc{/reference/network/http_values}
/// @tested{HttpValueTest}
class HttpMethod final {
public:
    /// Create an invalid placeholder.
    HttpMethod() noexcept = default;
    /// Create a recognized standard method, or an invalid placeholder for `None` and `All`.
    HttpMethod(HttpMethodType type) noexcept; // NOLINT(*-explicit-constructor)

    // defaults
    ~HttpMethod() = default;
    HttpMethod(const HttpMethod &) noexcept = default;
    HttpMethod(HttpMethod &&) noexcept = default;
    auto operator=(const HttpMethod &) noexcept -> HttpMethod & = default;
    auto operator=(HttpMethod &&) noexcept -> HttpMethod & = default;

public: // operators
    /// Compare method names exactly.
    [[nodiscard]] auto operator<=>(const HttpMethod &other) const noexcept -> std::strong_ordering = default;

public: // tests/accessors
    /// Test whether this method contains a valid token.
    [[nodiscard]] auto isValid() const noexcept -> bool { return !_text.isEmpty(); }
    /// Test whether this method is a recognized standard method.
    [[nodiscard]] auto isStandard() const noexcept -> bool { return _type != HttpMethodType::None; }
    /// Test whether this is a valid extension method.
    [[nodiscard]] auto isExtension() const noexcept -> bool { return isValid() && !isStandard(); }
    /// Get the recognized standard type, or no value for invalid and extension methods.
    [[nodiscard]] auto standardType() const noexcept -> std::optional<HttpMethodType>;
    /// Get the exact method token.
    [[nodiscard]] auto text() const noexcept -> const text::String & { return _text; }

public: // conversion
    /// Convert this method to its exact token.
    [[nodiscard]] auto toString() const noexcept -> text::String { return _text; }
    /// Parse a method token, returning an invalid placeholder on failure.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> HttpMethod;
    /// Parse a method token.
    /// @throws err::ParseError If the text is empty or is not an HTTP token.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> HttpMethod;

private:
    /// Create a validated method value.
    HttpMethod(text::String text, HttpMethodType type) noexcept : _text{std::move(text)}, _type{type} {}
    /// Classify an exact canonical method token.
    [[nodiscard]] static auto classify(const text::String &text) noexcept -> HttpMethodType;

private:
    text::String _text;                         ///< Exact case-sensitive method token.
    HttpMethodType _type{HttpMethodType::None}; ///< Recognized standard type.
};

}

namespace std {

template <>
struct hash<erbsland::network::HttpMethod> {
    auto operator()(const erbsland::network::HttpMethod &value) const noexcept -> std::size_t {
        return value.text().toHash();
    }
};

}
