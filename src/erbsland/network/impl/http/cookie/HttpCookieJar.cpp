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

auto HttpCookieJar::parsePeerCookie(const Url &url, const text::String &field) const
    -> std::optional<HttpCookieJarEntry> {
    auto parts = split(field, Char{';'});
    if (parts.empty()) {
        return {};
    }
    const auto equals = parts.front().findFirstOf(CharSet{Char{'='}});
    if (equals.isNoIndex()) {
        return {};
    }
    auto entry = HttpCookieJarEntry{};
    entry.name = parts.front().slice({}, unit::ByteLength{equals.toRawValue()}).trimmed();
    entry.value = parts.front().slice(StringSide::Back, equals.incremented()).trimmed();
    entry.domain = hostText(url);
    entry.path = defaultPath(url);
    entry.hostOnly = true;
    auto maxAgeSeen = false;
    auto expires = std::optional<time::DateTime>{};
    if (!isCookieToken(entry.name) || !isCookieValue(entry.value)) {
        return {};
    }
    for (auto index = std::size_t{1U}; index < parts.size(); ++index) {
        const auto part = parts[index].trimmed();
        const auto attributeEquals = part.findFirstOf(CharSet{Char{'='}});
        auto name = lowerAscii(
            attributeEquals.isNoIndex() ? part : part.slice({}, unit::ByteLength{attributeEquals.toRawValue()}));
        auto value = attributeEquals.isNoIndex()
            ? String{}
            : part.slice(StringSide::Back, attributeEquals.incremented()).trimmed();
        if (name == "domain"_el) {
            if (url.endpoint().host().isAddress()) {
                return {};
            }
            auto domain = value;
            if (domain.startsWith("."_el)) {
                domain = domain.slice(StringSide::Back, unit::ByteIndex{1U});
            }
            const auto domainName = HostName::fromString(domain);
            if (!domainName) {
                return {};
            }
            domain = domainName->toString(HostNameFormat::IdnaAscii);
            if (!domainMatches(entry.domain, domain) || public_suffix::isPublicSuffix(domain)) {
                return {};
            }
            entry.domain = std::move(domain);
            entry.hostOnly = false;
        } else if (name == "path"_el && value.startsWith("/"_el)) {
            entry.path = value;
        } else if (name == "secure"_el) {
            entry.secure = true;
        } else if (name == "httponly"_el) {
            entry.httpOnly = true;
        } else if (name == "samesite"_el) {
            const auto sameSite = lowerAscii(value);
            if (sameSite == "strict"_el) {
                entry.sameSite = HttpCookieSameSite::Strict;
            } else if (sameSite == "lax"_el) {
                entry.sameSite = HttpCookieSameSite::Lax;
            } else if (sameSite == "none"_el) {
                entry.sameSite = HttpCookieSameSite::None;
            }
        } else if (name == "max-age"_el) {
            try {
                const auto seconds = value.toIntegerOrThrow<std::int64_t>();
                entry.expires = seconds <= 0 ? time::DateTime::fromTimeT(0)
                                             : time::DateTime::now() + time::Duration{time::Seconds{seconds}};
                maxAgeSeen = true;
            } catch (...) {
                // Invalid Max-Age attributes are ignored; a valid Expires may still apply.
            }
        } else if (name == "expires"_el) {
            expires = parseCookieDate(value);
        }
    }
    if (!maxAgeSeen) {
        entry.expires = expires;
    }
    if (!validPrefix(entry, url.scheme() == UrlScheme::Https)) {
        return {};
    }
    return entry;
}

auto HttpCookieJar::hostText(const Url &url) -> text::String {
    const auto endpoint = url.endpoint();
    const auto &host = endpoint.host();
    if (const auto name = host.name()) {
        return name->toString(HostNameFormat::IdnaAscii);
    }
    return host.toString();
}

auto HttpCookieJar::defaultPath(const Url &url) -> text::String {
    const auto path = url.path();
    if (path.isEmpty() || !path.startsWith("/"_el)) {
        return "/"_el;
    }
    const auto slash = path.findLastOf(CharSet{Char{'/'}});
    if (slash.isNoIndex() || slash.toRawValue() == 0U) {
        return "/"_el;
    }
    return path.slice({}, unit::ByteLength{slash.toRawValue()});
}

auto HttpCookieJar::domainMatches(const text::String &host, const text::String &domain) noexcept -> bool {
    if (host == domain) {
        return true;
    }
    auto suffix = StringEditor{};
    suffix.append(U'.').append(domain);
    return host.endsWith(String{suffix});
}

auto HttpCookieJar::pathMatches(const text::String &requestPath, const text::String &cookiePath) noexcept -> bool {
    if (requestPath == cookiePath) {
        return true;
    }
    if (!requestPath.startsWith(cookiePath)) {
        return false;
    }
    return cookiePath.endsWith("/"_el) ||
        requestPath.charAt(unit::ByteIndex{cookiePath.length().toRawValue()}) == Char{'/'};
}

auto HttpCookieJar::isCookieToken(const text::String &value) noexcept -> bool {
    if (value.isEmpty()) {
        return false;
    }
    auto reader = StringCharReader{value};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!character.isAscii() || character.toRawValue() <= 0x20 || character.toRawValue() >= 0x7f ||
            CharSet{String{"()<>@,;:\\\"/[]?={}"}}.contains(character)) {
            return false;
        }
    }
    return true;
}

auto HttpCookieJar::isCookieValue(const text::String &value) noexcept -> bool {
    auto reader = StringCharReader{value};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        const auto code = character.toRawValue();
        if (!(code == 0x21 || (code >= 0x23 && code <= 0x2b) || (code >= 0x2d && code <= 0x3a) ||
                (code >= 0x3c && code <= 0x5b) || (code >= 0x5d && code <= 0x7e))) {
            return false;
        }
    }
    return true;
}

auto HttpCookieJar::lowerAscii(const text::String &value) -> text::String {
    auto result = StringEditor{};
    auto reader = StringCharReader{value};
    while (!reader.isAtEnd()) {
        result.append(reader.read().toAsciiLowercase());
    }
    return String{result};
}

auto HttpCookieJar::split(const text::String &value, const text::Char separator) -> std::vector<text::String> {
    auto result = std::vector<text::String>{};
    auto separatorText = StringEditor{};
    separatorText.append(separator);
    const auto needle = String{separatorText};
    auto start = unit::ByteIndex{};
    while (start.isWithin(value.length())) {
        const auto found = value.find(needle, start);
        if (found.isNoIndex()) {
            result.emplace_back(value.slice(unit::ByteRange{start, unit::ByteLength::infinite()}));
            return result;
        }
        result.emplace_back(
            value.slice(unit::ByteRange{start, unit::ByteLength{found.toRawValue() - start.toRawValue()}}));
        start = found.incremented();
    }
    result.emplace_back();
    return result;
}

auto HttpCookieJar::parseCookieDate(const text::String &value) noexcept -> std::optional<time::DateTime> {
    try {
        auto tokens = std::vector<String>{};
        auto reader = StringCharReader{value};
        auto token = StringEditor{};
        while (!reader.isAtEnd()) {
            const auto character = reader.read();
            const auto code = character.toRawValue();
            const auto delimiter = code == 0x09U || (code >= 0x20U && code <= 0x2fU) ||
                (code >= 0x3bU && code <= 0x40U) || (code >= 0x5bU && code <= 0x60U) ||
                (code >= 0x7bU && code <= 0x7eU);
            if (delimiter) {
                if (!token.isEmpty()) {
                    tokens.emplace_back(token);
                    token.clear();
                }
            } else {
                token.append(character);
            }
        }
        if (!token.isEmpty()) {
            tokens.emplace_back(token);
        }
        auto day = -1;
        auto month = -1;
        auto year = -1;
        auto hour = -1;
        auto minute = -1;
        auto second = -1;
        for (const auto &item : tokens) {
            if (hour < 0 && item.contains(":"_el)) {
                const auto timeParts = split(item, Char{':'});
                if (timeParts.size() == 3U) {
                    hour = timeParts[0].toInteger<int>(-1);
                    minute = timeParts[1].toInteger<int>(-1);
                    second = timeParts[2].toInteger<int>(-1);
                }
                continue;
            }
            const auto lower = lowerAscii(item);
            if (month < 0) {
                constexpr auto cMonths = std::array<text::StringLiteral, 12U>{
                    "jan"_el,
                    "feb"_el,
                    "mar"_el,
                    "apr"_el,
                    "may"_el,
                    "jun"_el,
                    "jul"_el,
                    "aug"_el,
                    "sep"_el,
                    "oct"_el,
                    "nov"_el,
                    "dec"_el};
                for (auto index = std::size_t{}; index < cMonths.size(); ++index) {
                    if (text::String{cMonths[index]} == lower) {
                        month = static_cast<int>(index) + 1;
                        break;
                    }
                }
                if (month > 0) {
                    continue;
                }
            }
            const auto number = item.toInteger<int>(-1);
            if (number < 0) {
                continue;
            }
            if (day < 0 && item.length() <= unit::ByteLength{2U} && number >= 1 && number <= 31) {
                day = number;
            } else if (year < 0 && item.length() >= unit::ByteLength{2U} && item.length() <= unit::ByteLength{4U}) {
                year = number;
            }
        }
        if (year >= 70 && year <= 99) {
            year += 1900;
        } else if (year >= 0 && year <= 69) {
            year += 2000;
        }
        if (year < 1601 || month < 1 || day < 1 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 ||
            second > 59) {
            return {};
        }
        const auto date = time::Date{time::Year{year}, time::Month{month}, time::Day{day}};
        if (!date.isValid()) {
            return {};
        }
        return time::DateTime{date, time::Time{time::Hour{hour}, time::Minute{minute}, time::Second{second}}};
    } catch (...) {
        return {};
    }
}

auto HttpCookieJar::validPrefix(const HttpCookieJarEntry &entry, const bool secureOrigin) noexcept -> bool {
    if (entry.name.startsWith("__Secure-"_el) && (!entry.secure || !secureOrigin)) {
        return false;
    }
    if (entry.name.startsWith("__Host-"_el) &&
        (!entry.secure || !secureOrigin || !entry.hostOnly || entry.path != "/"_el)) {
        return false;
    }
    return true;
}

}
