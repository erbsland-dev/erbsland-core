// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpHeaders.hpp"
#include "HttpStatus.hpp"
#include "HttpVersion.hpp"

#include "../../text/String.hpp"

#include <utility>

namespace erbsland::network {

/// The control data and header section of an HTTP response.
/// The reason phrase retains its exact bytes, including obs-text and malformed UTF-8.
/// @seedoc{/reference/network/http_messages}
/// @tested{HttpMessageTest Http1CodecTest}
class HttpResponseHead final {
public:
    /// Create an invalid placeholder.
    HttpResponseHead() noexcept = default;
    /// Create a validated response head.
    /// @param version The supported HTTP version.
    /// @param status The valid response status.
    /// @param reasonPhrase The exact optional reason phrase.
    /// @param headers The ordered response header fields.
    /// @throws err::ParameterError If a control value is invalid.
    HttpResponseHead(HttpVersion version, HttpStatus status, text::String reasonPhrase, HttpHeaders headers = {});

    // defaults
    ~HttpResponseHead() = default;
    HttpResponseHead(const HttpResponseHead &) noexcept = default;
    HttpResponseHead(HttpResponseHead &&) noexcept = default;
    auto operator=(const HttpResponseHead &) noexcept -> HttpResponseHead & = default;
    auto operator=(HttpResponseHead &&) noexcept -> HttpResponseHead & = default;

public: // operators
    /// Compare all response-head values.
    [[nodiscard]] auto operator==(const HttpResponseHead &other) const noexcept -> bool = default;

public: // tests/accessors
    /// Test whether the control data is valid.
    [[nodiscard]] auto isValid() const noexcept -> bool;
    /// Get the HTTP version.
    [[nodiscard]] auto version() const noexcept -> HttpVersion { return _version; }
    /// Get the response status.
    [[nodiscard]] auto status() const noexcept -> HttpStatus { return _status; }
    /// Get the exact reason phrase.
    [[nodiscard]] auto reasonPhrase() const noexcept -> const text::String & { return _reasonPhrase; }
    /// Get the ordered header fields.
    [[nodiscard]] auto headers() const noexcept -> const HttpHeaders & { return _headers; }

private:
    HttpVersion _version;       ///< Supported HTTP version.
    HttpStatus _status;         ///< Valid response status.
    text::String _reasonPhrase; ///< Exact optional reason phrase bytes.
    HttpHeaders _headers;       ///< Ordered response fields.
};

}
