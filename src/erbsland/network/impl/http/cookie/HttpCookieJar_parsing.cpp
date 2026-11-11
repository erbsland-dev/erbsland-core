// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpCookieJar.hpp"

#include "../../../../err/LogicFailure.hpp"
#include "../../../../err/ParameterFailure.hpp"
#include "../../../../err/RuntimeError.hpp"
#include "../../../../event/CurrentLoop.hpp"
#include "../../../../event/EventTarget.hpp"
#include "../../../../text/CharSet.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../../../text/StringEditor.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../../text/StringTransformations.hpp"
#include "../../../../time/Date.hpp"
#include "../../../../time/Time.hpp"
#include "../../../../unit/ItemCount.hpp"
#include "../../../HostName.hpp"
#include "../../../HostNameFormat.hpp"
#include "../../../http/HttpFieldType.hpp"

#include <algorithm>
#include <array>
#include <ranges>

namespace erbsland::network::impl {

using namespace unit;
using namespace text;
using namespace text::literals;

auto HttpCookieJar::parsePeerCookie(const Url &url, const String &field) const -> std::optional<HttpCookieJarEntry> {
    auto parts = StringList::fromSplit(field, CharSet{Char{';'}}, ItemCount::infinite(), true);
    if (parts.isEmpty()) {
        return {};
    }
    const auto fields = parts.first().splitAtFirst(CharSet{U'='});
    if (!fields) {
        return {};
    }
    auto entry = HttpCookieJarEntry{};
    entry.name = fields->first.trimmed();
    entry.value = fields->second.trimmed();
    entry.domain = hostText(url);
    entry.path = defaultPath(url);
    entry.hostOnly = true;
    auto maxAgeSeen = false;
    auto expires = std::optional<time::Timestamp>{};
    if (!isCookieToken(entry.name) || !isCookieValue(entry.value)) {
        return {};
    }
    for (auto index = ItemIndex{1U}; index.isWithin(parts.count()); index.uncheckedIncrement()) {
        const auto part = parts.getRef(index).trimmed();
        auto name = part;
        auto value = String{};
        if (const auto attribute = part.splitAtFirst(CharSet{U'='})) {
            name = attribute->first;
            value = attribute->second.trimmed();
        }
        name = text::toAsciiLowercase(name);
        if (name == "domain"_el) {
            if (url.endpoint().host().isAddress()) {
                return {};
            }
            auto domain = value;
            if (domain.startsWith("."_el)) {
                domain = domain.slice(StringSide::Back, ByteIndex{1U});
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
            const auto sameSite = text::toAsciiLowercase(value);
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
                entry.expires = seconds <= 0 ? time::Timestamp::fromTimeT(0)
                                             : time::Timestamp::now() + time::Duration{time::Seconds{seconds}};
                maxAgeSeen = true;
            } catch (const err::RuntimeError &) {
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

auto HttpCookieJar::hostText(const Url &url) -> String {
    const auto endpoint = url.endpoint();
    const auto &host = endpoint.host();
    if (const auto name = host.name()) {
        return name->toString(HostNameFormat::IdnaAscii);
    }
    return host.toString();
}

auto HttpCookieJar::defaultPath(const Url &url) -> String {
    const auto path = url.path();
    if (path.isEmpty() || !path.startsWith("/"_el)) {
        return "/"_el;
    }
    const auto parts = path.splitAtLast(CharSet{U'/'});
    if (!parts || parts->first.isEmpty()) {
        return "/"_el;
    }
    return parts->first;
}

auto HttpCookieJar::domainMatches(const String &host, const String &domain) noexcept -> bool {
    if (host == domain) {
        return true;
    }
    auto suffix = StringEditor{};
    suffix.append(U'.').append(domain);
    return host.endsWith(suffix);
}

auto HttpCookieJar::pathMatches(const String &requestPath, const String &cookiePath) noexcept -> bool {
    if (requestPath == cookiePath) {
        return true;
    }
    if (!requestPath.startsWith(cookiePath)) {
        return false;
    }
    return cookiePath.endsWith("/"_el) || requestPath.charAt(ByteIndex::end(cookiePath.length())) == Char{'/'};
}

auto HttpCookieJar::isCookieToken(const String &value) noexcept -> bool {
    static const auto cSeparators = CharSet{"()<>@,;:\\\"/[]?={}"_el};
    if (value.isEmpty()) {
        return false;
    }
    auto reader = StringCharReader{value};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!character.isAscii() || character.toRawValue() <= 0x20 || character.toRawValue() >= 0x7f ||
            cSeparators.contains(character)) {
            return false;
        }
    }
    return true;
}

auto HttpCookieJar::isCookieValue(const String &value) noexcept -> bool {
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

auto HttpCookieJar::parseCookieDate(const String &value) noexcept -> std::optional<time::Timestamp> {
    try {
        auto tokens = StringList{};
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
                    tokens.append(token);
                    token.clear();
                }
            } else {
                token.append(character);
            }
        }
        if (!token.isEmpty()) {
            tokens.append(token);
        }
        auto day = -1;
        auto month = -1;
        auto year = -1;
        auto hour = -1;
        auto minute = -1;
        auto second = -1;
        for (const auto &item : tokens) {
            if (hour < 0 && item.contains(":"_el)) {
                const auto timeParts = StringList::fromSplit(item, CharSet{Char{':'}});
                if (timeParts.count() == ItemCount{3U}) {
                    hour = timeParts.getRef(ItemIndex{0}).toInteger<int>(-1);
                    minute = timeParts.getRef(ItemIndex{1}).toInteger<int>(-1);
                    second = timeParts.getRef(ItemIndex{2}).toInteger<int>(-1);
                }
                continue;
            }
            const auto lower = text::toAsciiLowercase(item);
            if (month < 0) {
                constexpr auto cMonths = std::array<StringLiteral, 12U>{
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
                    if (String{cMonths[index]} == lower) {
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
            if (day < 0 && item.length() <= ByteCount{2U} && number >= 1 && number <= 31) {
                day = number;
            } else if (year < 0 && item.length() >= ByteCount{2U} && item.length() <= ByteCount{4U}) {
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
        return time::Timestamp{date, time::Time{time::Hour{hour}, time::Minute{minute}, time::Second{second}}};
    } catch (const err::RuntimeError &) {
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
