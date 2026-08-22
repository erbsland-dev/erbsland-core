// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpCookieJarEntry.hpp"
#include "PublicSuffixList.hpp"

#include "../../../../event/Events_fwd.hpp"
#include "../../../http/HttpHeaders.hpp"
#include "../../../http_client/HttpCookieJar.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace erbsland::network::impl {

/// Bounded event-loop-owned HTTP client cookie jar.
/// @tested{HttpCookieJarTest HttpClientTest}
class HttpCookieJar final : public network::HttpCookieJar {
public:
    /// Create a cookie jar owned by the given event collection.
    explicit HttpCookieJar(event::EventsPtr ownerEvents);

public: // implement network::HttpCookieJar
    [[nodiscard]] auto options() const noexcept -> HttpCookieJarOptions override;
    void setOptions(HttpCookieJarOptions options) override;
    [[nodiscard]] auto cookies() -> std::vector<HttpCookie> override;
    /// Insert or replace a manually supplied cookie.
    void setCookie(
        const Url &url, text::String name, text::String value, HttpCookieInsertionOptions options = {}) override;
    auto removeCookie(const text::String &name, const text::String &domain, const text::String &path) -> bool override;
    void clear() noexcept override;

public: // HTTP integration
    /// Store every valid Set-Cookie field, ignoring invalid peer input.
    void storeResponseCookies(const Url &url, const HttpHeaders &headers) noexcept;
    /// Store one Set-Cookie field and report whether it was accepted.
    auto storeResponseCookie(const Url &url, const text::String &field) noexcept -> bool;
    /// Build the deterministic Cookie field for one request URL.
    [[nodiscard]] auto requestCookieHeader(const Url &url) -> text::String;
    /// Return the canonical lowercase ASCII host text for a URL.
    [[nodiscard]] static auto hostText(const Url &url) -> text::String;

private:
    /// Verify event-loop ownership.
    void verifyOwner() const;
    /// Validate all finite cookie-jar limits.
    void validateOptions(const HttpCookieJarOptions &options) const;
    /// Insert or replace one validated cookie entry.
    void insert(HttpCookieJarEntry entry, bool peerInput);
    /// Remove expired entries using the current wall-clock time.
    void purgeExpired();
    /// Enforce per-domain and global limits through LRU eviction.
    void enforceLimits();
    /// Evict the least-recently-used entry, optionally within one registrable domain.
    void evictOne(const std::optional<text::String> &registrableDomain = {});
    /// Parse and validate one peer Set-Cookie field.
    [[nodiscard]] auto parsePeerCookie(const Url &url, const text::String &field) const
        -> std::optional<HttpCookieJarEntry>;
    /// Derive the RFC cookie default path for a URL.
    [[nodiscard]] static auto defaultPath(const Url &url) -> text::String;
    /// Test RFC domain matching for canonical host and domain text.
    [[nodiscard]] static auto domainMatches(const text::String &host, const text::String &domain) noexcept -> bool;
    /// Test RFC path matching.
    [[nodiscard]] static auto pathMatches(const text::String &requestPath, const text::String &cookiePath) noexcept
        -> bool;
    /// Test whether text is a valid cookie-name token.
    [[nodiscard]] static auto isCookieToken(const text::String &value) noexcept -> bool;
    /// Test whether text is a valid cookie value.
    [[nodiscard]] static auto isCookieValue(const text::String &value) noexcept -> bool;
    /// Create an ASCII-lowercase copy.
    [[nodiscard]] static auto lowerAscii(const text::String &value) -> text::String;
    /// Split text at one separator.
    [[nodiscard]] static auto split(const text::String &value, text::Char separator) -> std::vector<text::String>;
    /// Parse one supported HTTP cookie date.
    [[nodiscard]] static auto parseCookieDate(const text::String &value) noexcept -> std::optional<time::DateTime>;
    /// Validate __Secure- and __Host- cookie prefix constraints.
    [[nodiscard]] static auto validPrefix(const HttpCookieJarEntry &entry, bool secureOrigin) noexcept -> bool;

private:
    event::EventsPtr _ownerEvents;
    HttpCookieJarOptions _options;
    std::vector<HttpCookieJarEntry> _entries;
    std::uint64_t _sequence{};
};

}
