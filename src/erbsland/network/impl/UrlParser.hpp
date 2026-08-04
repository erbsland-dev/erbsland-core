// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UrlData.hpp"

#include "../url/UrlParseOptions.hpp"

#include <memory>
#include <optional>
#include <utility>

namespace erbsland::network::impl {

/// Parse and construct URL data.
/// @tested{UrlTest}
class UrlParser final {
public:
    /// Create a parser for one absolute URL.
    UrlParser(text::String text, UrlParseOptions options) : _text{std::move(text)}, _options{options} {}
    /// Parse the configured absolute URL.
    [[nodiscard]] auto parse() const -> std::shared_ptr<UrlData>;
    /// Construct a validated host-network URL.
    [[nodiscard]] static auto network(
        UrlScheme scheme, HostEndpoint endpoint, text::String path, text::String query, text::String fragment)
        -> std::shared_ptr<UrlData>;
    /// Construct a validated file URL.
    [[nodiscard]] static auto file(
        std::optional<Host> host, text::String path, text::String query, text::String fragment)
        -> std::shared_ptr<UrlData>;
    /// Construct a validated mailto URL.
    [[nodiscard]] static auto mailto(text::String address, text::String query, text::String fragment)
        -> std::shared_ptr<UrlData>;
    /// Construct a validated custom URL.
    [[nodiscard]] static auto custom(
        text::String scheme,
        std::optional<text::String> authority,
        text::String path,
        text::String query,
        text::String fragment) -> std::shared_ptr<UrlData>;

private:
    /// Recognize a canonical scheme name.
    [[nodiscard]] static auto schemeFromText(const text::String &text) noexcept -> UrlScheme;
    /// Get canonical text for a built-in scheme.
    [[nodiscard]] static auto schemeText(UrlScheme scheme) -> text::String;
    /// Get the inferred port for a host-network scheme.
    [[nodiscard]] static auto defaultPort(UrlScheme scheme) noexcept -> Port;
    /// Strictly percent-decode and normalize one component.
    [[nodiscard]] static auto decodeComponent(const text::String &text) -> text::String;
    /// Validate and normalize a constructed component.
    [[nodiscard]] static auto normalizedComponent(text::String text) -> text::String;
    /// Parse the authority of a known host-network scheme.
    static void parseKnownAuthority(UrlData &data, Port defaultPort);
    /// Parse the restricted authority of a file URL.
    static void parseFileAuthority(UrlData &data);
    /// Parse a custom authority without propagating endpoint errors.
    static void parseCustomAuthority(UrlData &data) noexcept;
    /// Split and decode user information from an authority.
    static void parseUserInfo(UrlData &data, text::String &hostPort);
    /// Parse a host and optional port.
    [[nodiscard]] static auto parseHostPort(const text::String &text, Port defaultPort, bool requirePort)
        -> HostEndpoint;
    /// Validate a custom scheme name.
    static void validateSchemeText(const text::String &scheme);
    /// Validate a mailto address component.
    static void validateMailtoPath(const text::String &path);
    /// Validate a path following an authority.
    static void validateAuthorityPath(const text::String &path);
    /// Validate an opaque authority supplied to a factory.
    static void validateAuthorityText(const text::String &authority);

private:
    text::String _text;       ///< Complete URL text.
    UrlParseOptions _options; ///< Active safety limits.
};

}
