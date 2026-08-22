// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpFieldType.hpp"

#include "../impl/http/HttpGrammar.hpp"

#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text::literals;

auto HttpFieldType::isRequestField() const noexcept -> bool {
    switch (_value) {
    case AcceptRanges:
    case Age:
    case Allow:
    case AuthenticationInfo:
    case ContentRange:
    case ETag:
    case Expires:
    case LastModified:
    case Location:
    case ProxyAuthenticate:
    case ProxyAuthenticationInfo:
    case RetryAfter:
    case Server:
    case SetCookie:
    case Vary:
    case WwwAuthenticate:
        return false;
    default:
        return true;
    }
}

auto HttpFieldType::isResponseField() const noexcept -> bool {
    switch (_value) {
    case Accept:
    case AcceptCharset:
    case AcceptEncoding:
    case AcceptLanguage:
    case Authorization:
    case Cookie:
    case Expect:
    case From:
    case Host:
    case IfMatch:
    case IfModifiedSince:
    case IfNoneMatch:
    case IfRange:
    case IfUnmodifiedSince:
    case MaxForwards:
    case ProxyAuthorization:
    case Range:
    case Referer:
    case Te:
    case UserAgent:
        return false;
    default:
        return true;
    }
}

auto HttpFieldType::toString() const noexcept -> text::String {
    switch (_value) {
    case Accept:
        return "Accept"_el;
    case AcceptCharset:
        return "Accept-Charset"_el;
    case AcceptEncoding:
        return "Accept-Encoding"_el;
    case AcceptLanguage:
        return "Accept-Language"_el;
    case AcceptRanges:
        return "Accept-Ranges"_el;
    case Age:
        return "Age"_el;
    case Allow:
        return "Allow"_el;
    case AuthenticationInfo:
        return "Authentication-Info"_el;
    case Authorization:
        return "Authorization"_el;
    case CacheControl:
        return "Cache-Control"_el;
    case Connection:
        return "Connection"_el;
    case ContentEncoding:
        return "Content-Encoding"_el;
    case ContentLanguage:
        return "Content-Language"_el;
    case ContentLength:
        return "Content-Length"_el;
    case ContentLocation:
        return "Content-Location"_el;
    case ContentRange:
        return "Content-Range"_el;
    case ContentType:
        return "Content-Type"_el;
    case Cookie:
        return "Cookie"_el;
    case Date:
        return "Date"_el;
    case ETag:
        return "ETag"_el;
    case Expect:
        return "Expect"_el;
    case Expires:
        return "Expires"_el;
    case From:
        return "From"_el;
    case Host:
        return "Host"_el;
    case IfMatch:
        return "If-Match"_el;
    case IfModifiedSince:
        return "If-Modified-Since"_el;
    case IfNoneMatch:
        return "If-None-Match"_el;
    case IfRange:
        return "If-Range"_el;
    case IfUnmodifiedSince:
        return "If-Unmodified-Since"_el;
    case LastModified:
        return "Last-Modified"_el;
    case Location:
        return "Location"_el;
    case MaxForwards:
        return "Max-Forwards"_el;
    case Pragma:
        return "Pragma"_el;
    case ProxyAuthenticate:
        return "Proxy-Authenticate"_el;
    case ProxyAuthenticationInfo:
        return "Proxy-Authentication-Info"_el;
    case ProxyAuthorization:
        return "Proxy-Authorization"_el;
    case Range:
        return "Range"_el;
    case Referer:
        return "Referer"_el;
    case RetryAfter:
        return "Retry-After"_el;
    case Server:
        return "Server"_el;
    case SetCookie:
        return "Set-Cookie"_el;
    case Te:
        return "TE"_el;
    case Trailer:
        return "Trailer"_el;
    case TransferEncoding:
        return "Transfer-Encoding"_el;
    case Upgrade:
        return "Upgrade"_el;
    case UserAgent:
        return "User-Agent"_el;
    case Vary:
        return "Vary"_el;
    case Via:
        return "Via"_el;
    case Warning:
        return "Warning"_el;
    case WwwAuthenticate:
        return "WWW-Authenticate"_el;
    case Unknown:
        return {};
    }
    return {};
}

auto HttpFieldType::fromString(const text::String &name) noexcept -> HttpFieldType {
    for (auto raw = static_cast<uint8_t>(Accept); raw <= static_cast<uint8_t>(WwwAuthenticate); ++raw) {
        const auto candidate = HttpFieldType{static_cast<Value>(raw)};
        if (impl::http_grammar::equalTokenCI(candidate.toString(), name)) {
            return candidate;
        }
    }
    return Unknown;
}

}
