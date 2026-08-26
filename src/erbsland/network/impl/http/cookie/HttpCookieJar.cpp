// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpCookieJar.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../event/CurrentEvents.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../text/CharSet.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../../../text/StringEditor.hpp"
#include "../../../../time/Date.hpp"
#include "../../../../time/Time.hpp"
#include "../../../HostName.hpp"
#include "../../../HostNameFormat.hpp"
#include "../../../http/HttpFieldType.hpp"

#include <algorithm>
#include <array>
#include <ranges>

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

HttpCookieJar::HttpCookieJar(event::EventsPtr ownerEvents) : _ownerEvents{std::move(ownerEvents)} {
}

auto HttpCookieJar::options() const noexcept -> HttpCookieJarOptions {
    return _options;
}

void HttpCookieJar::setOptions(HttpCookieJarOptions options) {
    verifyOwner();
    validateOptions(options);
    _options = options;
    enforceLimits();
}

auto HttpCookieJar::cookies() -> std::vector<HttpCookie> {
    verifyOwner();
    purgeExpired();
    auto sorted = _entries;
    std::ranges::sort(sorted, {}, &HttpCookieJarEntry::created);
    auto result = std::vector<HttpCookie>{};
    result.reserve(sorted.size());
    for (const auto &entry : sorted) {
        result.emplace_back(
            entry.name,
            entry.value,
            entry.domain,
            entry.path,
            entry.expires,
            entry.sameSite,
            entry.hostOnly,
            entry.secure,
            entry.httpOnly);
    }
    return result;
}

void HttpCookieJar::setCookie(
    const Url &url, text::String name, text::String value, HttpCookieInsertionOptions options) {
    verifyOwner();
    if (!url.isValid() || (url.scheme() != UrlScheme::Http && url.scheme() != UrlScheme::Https) ||
        !isCookieToken(name) || !isCookieValue(value)) {
        throw err::ParameterError{"A valid HTTP URL, cookie name, and cookie value are required."_el, "cookie"_el};
    }
    auto entry = HttpCookieJarEntry{};
    entry.name = std::move(name);
    entry.value = std::move(value);
    entry.domain = hostText(url);
    entry.path = defaultPath(url);
    entry.hostOnly = true;
    entry.secure = options.isSecure();
    entry.httpOnly = options.isHttpOnly();
    entry.expires = options.expires();
    entry.sameSite = options.sameSite();
    if (options.domain()) {
        if (url.endpoint().host().isAddress()) {
            throw err::ParameterError{"IP-address cookies cannot use a Domain attribute."_el, "domain"_el};
        }
        auto domain = options.domain()->trimmed();
        if (domain.startsWith("."_el)) {
            domain = domain.slice(StringSide::Back, unit::ByteIndex{1U});
        }
        const auto domainName = HostName::fromString(domain);
        if (!domainName) {
            throw err::ParameterError{"The cookie Domain is not a valid host name."_el, "domain"_el};
        }
        domain = domainName->toString(HostNameFormat::IdnaAscii);
        if (!domainMatches(entry.domain, domain) || public_suffix::isPublicSuffix(domain)) {
            throw err::ParameterError{"The cookie Domain is not valid for the URL."_el, "domain"_el};
        }
        entry.domain = std::move(domain);
        entry.hostOnly = false;
    }
    if (options.path()) {
        if (!options.path()->startsWith("/"_el)) {
            throw err::ParameterError{"A cookie Path must start with '/'."_el, "path"_el};
        }
        entry.path = *options.path();
    }
    if (!validPrefix(entry, url.scheme() == UrlScheme::Https)) {
        throw err::ParameterError{"The cookie violates a __Secure- or __Host- prefix rule."_el, "name"_el};
    }
    insert(std::move(entry), false);
}

auto HttpCookieJar::removeCookie(const text::String &name, const text::String &domain, const text::String &path)
    -> bool {
    verifyOwner();
    const auto canonicalDomain = lowerAscii(domain);
    const auto previous = _entries.size();
    std::erase_if(_entries, [&](const HttpCookieJarEntry &entry) -> bool {
        return entry.name == name && entry.domain == canonicalDomain && entry.path == path;
    });
    return _entries.size() != previous;
}

void HttpCookieJar::clear() noexcept {
    _entries.clear();
}

void HttpCookieJar::storeResponseCookies(const Url &url, const HttpHeaders &headers) noexcept {
    try {
        verifyOwner();
    } catch (...) {
        return;
    }
    for (const auto &field : headers.getAll(HttpFieldType::SetCookie)) {
        storeResponseCookie(url, field);
    }
}

auto HttpCookieJar::storeResponseCookie(const Url &url, const text::String &field) noexcept -> bool {
    try {
        verifyOwner();
        if (field.length() > _options.maximumCookieLength()) {
            return false;
        }
        if (auto entry = parsePeerCookie(url, field)) {
            insert(std::move(*entry), true);
            return true;
        }
    } catch (const err::Exception &) {
        return false;
    }
    return false;
}

auto HttpCookieJar::requestCookieHeader(const Url &url) -> text::String {
    verifyOwner();
    purgeExpired();
    const auto host = hostText(url);
    auto requestPath = url.path();
    if (requestPath.isEmpty()) {
        requestPath = "/"_el;
    }
    auto matching = std::vector<HttpCookieJarEntry *>{};
    for (auto &entry : _entries) {
        if ((entry.hostOnly ? entry.domain == host : domainMatches(host, entry.domain)) &&
            pathMatches(requestPath, entry.path) && (!entry.secure || url.scheme() == UrlScheme::Https)) {
            entry.accessed = ++_sequence;
            matching.emplace_back(&entry);
        }
    }
    std::ranges::sort(matching, [](const HttpCookieJarEntry *first, const HttpCookieJarEntry *second) -> bool {
        if (first->path.length() != second->path.length()) {
            return first->path.length() > second->path.length();
        }
        return first->created < second->created;
    });
    auto result = StringEditor{};
    for (const auto *entry : matching) {
        if (!result.isEmpty()) {
            result.append("; "_el);
        }
        result.append(entry->name).append("="_el).append(entry->value);
    }
    return String{result};
}

void HttpCookieJar::verifyOwner() const {
    if (event::currentEvents() != _ownerEvents) {
        throw err::LogicError{"HTTP cookie jars are owned by their session event loop."_el};
    }
}

void HttpCookieJar::validateOptions(const HttpCookieJarOptions &options) const {
    if (options.maximumCookieLength().isZero() || options.maximumCookieLength().isInfinite() ||
        options.maximumCookiesPerRegistrableDomain().isZero() ||
        options.maximumCookiesPerRegistrableDomain().isInfinite() || options.maximumCookies().isZero() ||
        options.maximumCookies().isInfinite()) {
        throw err::ParameterError{"Cookie jar limits must be positive and finite."_el, "options"_el};
    }
}

void HttpCookieJar::insert(HttpCookieJarEntry entry, const bool peerInput) {
    const auto serializedLength =
        entry.name.length() + entry.value.length() + entry.domain.length() + entry.path.length();
    if (serializedLength > _options.maximumCookieLength() || entry.domain.isEmpty() || entry.path.isEmpty()) {
        if (peerInput) {
            return;
        }
        throw err::ParameterError{"The cookie exceeds the configured limit."_el, "cookie"_el};
    }
    entry.registrableDomain =
        Host::fromString(entry.domain)->isAddress() ? entry.domain : public_suffix::registrableDomain(entry.domain);
    if (entry.registrableDomain.isEmpty()) {
        entry.registrableDomain = entry.domain;
    }
    const auto now = time::DateTime::now();
    if (entry.expires && *entry.expires <= now) {
        removeCookie(entry.name, entry.domain, entry.path);
        return;
    }
    const auto existing = std::ranges::find_if(_entries, [&](const HttpCookieJarEntry &candidate) -> bool {
        return candidate.name == entry.name && candidate.domain == entry.domain && candidate.path == entry.path;
    });
    if (existing != _entries.end()) {
        entry.created = existing->created;
        *existing = std::move(entry);
        existing->accessed = ++_sequence;
    } else {
        entry.created = entry.accessed = ++_sequence;
        _entries.emplace_back(std::move(entry));
    }
    enforceLimits();
}

void HttpCookieJar::purgeExpired() {
    const auto now = time::DateTime::now();
    std::erase_if(
        _entries, [&](const HttpCookieJarEntry &entry) -> bool { return entry.expires && *entry.expires <= now; });
}

void HttpCookieJar::enforceLimits() {
    purgeExpired();
    for (;;) {
        auto overDomain = std::optional<text::String>{};
        for (const auto &entry : _entries) {
            const auto count = static_cast<std::size_t>(
                std::ranges::count(_entries, entry.registrableDomain, &HttpCookieJarEntry::registrableDomain));
            if (count > _options.maximumCookiesPerRegistrableDomain().toSizeT()) {
                overDomain = entry.registrableDomain;
                break;
            }
        }
        if (!overDomain) {
            break;
        }
        evictOne(overDomain);
    }
    while (_entries.size() > _options.maximumCookies().toSizeT()) {
        evictOne();
    }
}

void HttpCookieJar::evictOne(const std::optional<text::String> &registrableDomain) {
    auto oldest = _entries.end();
    for (auto it = _entries.begin(); it != _entries.end(); ++it) {
        if (registrableDomain && it->registrableDomain != *registrableDomain) {
            continue;
        }
        if (oldest == _entries.end() || it->accessed < oldest->accessed) {
            oldest = it;
        }
    }
    if (oldest != _entries.end()) {
        _entries.erase(oldest);
    }
}

}
