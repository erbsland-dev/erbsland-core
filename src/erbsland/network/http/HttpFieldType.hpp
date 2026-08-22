// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <cstdint>

namespace erbsland::network {

/// A recognized HTTP field-name classification.
/// Applicability metadata is descriptive and never makes an extension field invalid.
/// @seedoc{/reference/network/http_values}
/// @tested{HttpHeadersTest}
class HttpFieldType final {
public:
    /// Recognized field names from the HTTP core and cookie specifications.
    enum Value : uint8_t {
        Unknown = 0U,            ///< An unrecognized extension field.
        Accept,                  ///< The Accept request field.
        AcceptCharset,           ///< The Accept-Charset request field.
        AcceptEncoding,          ///< The Accept-Encoding field.
        AcceptLanguage,          ///< The Accept-Language request field.
        AcceptRanges,            ///< The Accept-Ranges response field.
        Age,                     ///< The Age response field.
        Allow,                   ///< The Allow response field.
        AuthenticationInfo,      ///< The Authentication-Info response field.
        Authorization,           ///< The Authorization request field.
        CacheControl,            ///< The Cache-Control field.
        Connection,              ///< The Connection field.
        ContentEncoding,         ///< The Content-Encoding representation field.
        ContentLanguage,         ///< The Content-Language representation field.
        ContentLength,           ///< The Content-Length field.
        ContentLocation,         ///< The Content-Location representation field.
        ContentRange,            ///< The Content-Range representation field.
        ContentType,             ///< The Content-Type representation field.
        Cookie,                  ///< The Cookie request field.
        Date,                    ///< The Date field.
        ETag,                    ///< The ETag response field.
        Expect,                  ///< The Expect request field.
        Expires,                 ///< The Expires response field.
        From,                    ///< The From request field.
        Host,                    ///< The Host request field.
        IfMatch,                 ///< The If-Match request field.
        IfModifiedSince,         ///< The If-Modified-Since request field.
        IfNoneMatch,             ///< The If-None-Match request field.
        IfRange,                 ///< The If-Range request field.
        IfUnmodifiedSince,       ///< The If-Unmodified-Since request field.
        LastModified,            ///< The Last-Modified response field.
        Location,                ///< The Location response field.
        MaxForwards,             ///< The Max-Forwards request field.
        Pragma,                  ///< The Pragma field.
        ProxyAuthenticate,       ///< The Proxy-Authenticate response field.
        ProxyAuthenticationInfo, ///< The Proxy-Authentication-Info response field.
        ProxyAuthorization,      ///< The Proxy-Authorization request field.
        Range,                   ///< The Range request field.
        Referer,                 ///< The Referer request field.
        RetryAfter,              ///< The Retry-After response field.
        Server,                  ///< The Server response field.
        SetCookie,               ///< The Set-Cookie response field.
        Te,                      ///< The TE request field.
        Trailer,                 ///< The Trailer field.
        TransferEncoding,        ///< The Transfer-Encoding field.
        Upgrade,                 ///< The Upgrade field.
        UserAgent,               ///< The User-Agent request field.
        Vary,                    ///< The Vary response field.
        Via,                     ///< The Via field.
        Warning,                 ///< The Warning field.
        WwwAuthenticate,         ///< The WWW-Authenticate response field.
    };

public:
    /// Create a field type from a value.
    constexpr HttpFieldType(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr HttpFieldType() noexcept = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const HttpFieldType &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const HttpFieldType &other, value, other._value);

public: // tests/accessors
    /// Test whether this type is recognized.
    [[nodiscard]] constexpr auto isKnown() const noexcept -> bool { return _value != Unknown; }
    /// Test whether this field is defined for requests.
    [[nodiscard]] auto isRequestField() const noexcept -> bool;
    /// Test whether this field is defined for responses.
    [[nodiscard]] auto isResponseField() const noexcept -> bool;
    /// Get the raw classification value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Get the conventional field-name spelling, or empty text for `Unknown`.
    [[nodiscard]] auto toString() const noexcept -> text::String;
    /// Classify a valid field name using ASCII-case-insensitive matching.
    [[nodiscard]] static auto fromString(const text::String &name) noexcept -> HttpFieldType;

private:
    Value _value{Unknown}; ///< Recognized field classification.
};

}
