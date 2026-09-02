// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpHeaders.hpp"
#include "HttpMethod.hpp"
#include "HttpVersion.hpp"

#include "../../text/String.hpp"

#include <utility>

namespace erbsland::network {

/// The control data and header section of an HTTP request.
/// The request-target retains its exact validated ASCII wire spelling.
/// @seedoc{/reference/network/http_protocol}
/// @tested{HttpMessageTest Http1CodecTest}
class HttpRequestHead final {
public:
    /// Create an invalid placeholder.
    HttpRequestHead() noexcept = default;
    /// Create a validated request head.
    /// @param method The valid request method.
    /// @param target The exact nonempty ASCII request-target.
    /// @param version The supported HTTP version.
    /// @param headers The ordered request header fields.
    /// @throws err::ParameterError If a control value is invalid.
    HttpRequestHead(HttpMethod method, text::String target, HttpVersion version, HttpHeaders headers = {});

    // defaults
    ~HttpRequestHead() = default;
    HttpRequestHead(const HttpRequestHead &) noexcept = default;
    HttpRequestHead(HttpRequestHead &&) noexcept = default;
    auto operator=(const HttpRequestHead &) noexcept -> HttpRequestHead & = default;
    auto operator=(HttpRequestHead &&) noexcept -> HttpRequestHead & = default;

public: // operators
    /// Compare all request-head values.
    [[nodiscard]] auto operator==(const HttpRequestHead &other) const noexcept -> bool = default;

public: // tests/accessors
    /// Test whether the control data is valid.
    [[nodiscard]] auto isValid() const noexcept -> bool;
    /// Get the exact method.
    [[nodiscard]] auto method() const noexcept -> const HttpMethod & { return _method; }
    /// Get the exact ASCII request-target.
    [[nodiscard]] auto target() const noexcept -> const text::String & { return _target; }
    /// Get the HTTP version.
    [[nodiscard]] auto version() const noexcept -> HttpVersion { return _version; }
    /// Get the ordered header fields.
    [[nodiscard]] auto headers() const noexcept -> const HttpHeaders & { return _headers; }

private:
    HttpMethod _method;   ///< Exact request method.
    text::String _target; ///< Exact request-target bytes.
    HttpVersion _version; ///< Supported HTTP version.
    HttpHeaders _headers; ///< Ordered request fields.
};

}
