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
