// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Url_fwd.hpp"
#include "UrlFormatOptions.hpp"
#include "UrlParseOptions.hpp"
#include "UrlScheme.hpp"

#include "../HostEndpoint.hpp"
#include "../impl/http/client/HttpClientRequest_fwd.hpp"
#include "../impl/url/UrlData_fwd.hpp"

#include <memory>
#include <utility>

namespace erbsland::network {

/// An absolute URL value for network operations.
/// Copies share immutable parsed data. Text components are decoded, NFC-normalized values; serialization is canonical
/// and does not preserve the original percent-encoding spelling.
/// @seedoc{/reference/network/addressing_and_urls}
/// @tested{UrlTest}
class Url final {
    friend class impl::HttpClientRequest;

public:
    /// Create an invalid URL placeholder.
    Url() noexcept = default;
    /// Create a secure HTTP URL.
    Url(HostEndpoint endpoint, text::String path = {}, text::String query = {}, text::String fragment = {});
    /// Create an HTTP, HTTPS, FTP, or FTPS URL.
    /// @throws err::ParameterError If the scheme or endpoint is invalid for a network URL.
    Url(UrlScheme scheme,
        HostEndpoint endpoint,
        text::String path = {},
        text::String query = {},
        text::String fragment = {});

    // defaults
    ~Url() = default;
    Url(const Url &) noexcept = default;
    Url(Url &&) noexcept = default;
    auto operator=(const Url &) noexcept -> Url & = default;
    auto operator=(Url &&) noexcept -> Url & = default;

public: // tests/accessors
    /// Test whether this URL is valid.
    [[nodiscard]] auto isValid() const noexcept -> bool;
    /// Get the parsed scheme.
    [[nodiscard]] auto scheme() const noexcept -> UrlScheme;
    /// Get the canonical lowercase scheme text.
    [[nodiscard]] auto schemeText() const noexcept -> text::String;
    /// Test whether the built-in scheme uses a secure transport.
    [[nodiscard]] auto isSecureScheme() const noexcept -> bool;
    /// Get the parsed endpoint, or the default endpoint if none was parsed.
    [[nodiscard]] auto endpoint() const noexcept -> HostEndpoint;
    /// Get the original or constructed authority text.
    [[nodiscard]] auto authorityText() const noexcept -> text::String;
    /// Get the decoded username.
    [[nodiscard]] auto username() const noexcept -> text::String;
    /// Get the decoded password.
    [[nodiscard]] auto password() const noexcept -> text::String;
    /// Get the decoded NFC path.
    [[nodiscard]] auto path() const noexcept -> text::String;
    /// Get the decoded NFC query.
    [[nodiscard]] auto query() const noexcept -> text::String;
    /// Test whether a query delimiter is present, including an explicitly empty query.
    [[nodiscard]] auto hasQuery() const noexcept -> bool;
    /// Get the decoded NFC fragment.
    [[nodiscard]] auto fragment() const noexcept -> text::String;
    /// Test whether a fragment delimiter is present, including an explicitly empty fragment.
    [[nodiscard]] auto hasFragment() const noexcept -> bool;

public: // conversion
    /// Format this URL in canonical form.
    [[nodiscard]] auto toString(UrlFormatOptions options = {}) const -> text::String;
    /// Parse an absolute URL, returning an invalid URL on failure.
    [[nodiscard]] static auto fromString(const text::String &text, UrlParseOptions options = {}) noexcept -> Url;
    /// Parse an absolute URL.
    /// @throws err::ParseError If the URL is invalid or exceeds configured limits.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text, UrlParseOptions options = {}) -> Url;
    /// Resolve a relative or absolute URI reference, returning an invalid URL on failure.
    [[nodiscard]] auto resolved(const text::String &reference, UrlParseOptions options = {}) const noexcept -> Url;
    /// Resolve a relative or absolute URI reference.
    /// @throws err::ParseError If the base or reference is invalid, unsupported, or exceeds configured limits.
    [[nodiscard]] auto resolvedOrThrow(const text::String &reference, UrlParseOptions options = {}) const -> Url;

public: // factories
    /// Create a local file URL.
    [[nodiscard]] static auto file(text::String path, text::String query = {}, text::String fragment = {}) -> Url;
    /// Create a hosted file URL.
    [[nodiscard]] static auto file(Host host, text::String path, text::String query = {}, text::String fragment = {})
        -> Url;
    /// Create a mailto URL.
    [[nodiscard]] static auto mailto(text::String address, text::String query = {}, text::String fragment = {}) -> Url;
    /// Create a custom path-only URL.
    [[nodiscard]] static auto custom(
        text::String scheme, text::String path = {}, text::String query = {}, text::String fragment = {}) -> Url;
    /// Create a custom URL with an opaque authority.
    [[nodiscard]] static auto customWithAuthority(
        text::String scheme,
        text::String authority,
        text::String path = {},
        text::String query = {},
        text::String fragment = {}) -> Url;

private:
    /// Create a URL from validated internal data.
    explicit Url(std::shared_ptr<impl::UrlData> data) noexcept : _data{std::move(data)} {}

private:
    std::shared_ptr<impl::UrlData> _data; ///< Shared immutable URL data.
};

}
