// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <cstdint>

namespace erbsland::network {

/// A valid HTTP status code or an invalid placeholder.
/// The value accepts extension codes from 100 through 599. Named values follow RFC 9110 plus selected registered codes.
/// @seedoc{/reference/network/http_values}
/// @tested{HttpValueTest}
class HttpStatus final {
public:
    /// Known HTTP status codes.
    enum Value : uint16_t {
        Invalid = 0U,                       ///< No valid status.
        Continue = 100U,                    ///< Continue.
        SwitchingProtocols = 101U,          ///< Switching Protocols.
        EarlyHints = 103U,                  ///< Early Hints.
        Ok = 200U,                          ///< OK.
        Created = 201U,                     ///< Created.
        Accepted = 202U,                    ///< Accepted.
        NonAuthoritativeInformation = 203U, ///< Non-Authoritative Information.
        NoContent = 204U,                   ///< No Content.
        ResetContent = 205U,                ///< Reset Content.
        PartialContent = 206U,              ///< Partial Content.
        MultipleChoices = 300U,             ///< Multiple Choices.
        MovedPermanently = 301U,            ///< Moved Permanently.
        Found = 302U,                       ///< Found.
        SeeOther = 303U,                    ///< See Other.
        NotModified = 304U,                 ///< Not Modified.
        UseProxy = 305U,                    ///< Use Proxy (deprecated).
        TemporaryRedirect = 307U,           ///< Temporary Redirect.
        PermanentRedirect = 308U,           ///< Permanent Redirect.
        BadRequest = 400U,                  ///< Bad Request.
        Unauthorized = 401U,                ///< Unauthorized.
        PaymentRequired = 402U,             ///< Payment Required.
        Forbidden = 403U,                   ///< Forbidden.
        NotFound = 404U,                    ///< Not Found.
        MethodNotAllowed = 405U,            ///< Method Not Allowed.
        NotAcceptable = 406U,               ///< Not Acceptable.
        ProxyAuthenticationRequired = 407U, ///< Proxy Authentication Required.
        RequestTimeout = 408U,              ///< Request Timeout.
        Conflict = 409U,                    ///< Conflict.
        Gone = 410U,                        ///< Gone.
        LengthRequired = 411U,              ///< Length Required.
        PreconditionFailed = 412U,          ///< Precondition Failed.
        ContentTooLarge = 413U,             ///< Content Too Large.
        UriTooLong = 414U,                  ///< URI Too Long.
        UnsupportedMediaType = 415U,        ///< Unsupported Media Type.
        RangeNotSatisfiable = 416U,         ///< Range Not Satisfiable.
        ExpectationFailed = 417U,           ///< Expectation Failed.
        MisdirectedRequest = 421U,          ///< Misdirected Request.
        UnprocessableContent = 422U,        ///< Unprocessable Content.
        UpgradeRequired = 426U,             ///< Upgrade Required.
        TooManyRequests = 429U,             ///< Too Many Requests.
        RequestHeaderFieldsTooLarge = 431U, ///< Request Header Fields Too Large.
        UnavailableForLegalReasons = 451U,  ///< Unavailable For Legal Reasons.
        InternalServerError = 500U,         ///< Internal Server Error.
        NotImplemented = 501U,              ///< Not Implemented.
        BadGateway = 502U,                  ///< Bad Gateway.
        ServiceUnavailable = 503U,          ///< Service Unavailable.
        GatewayTimeout = 504U,              ///< Gateway Timeout.
        HttpVersionNotSupported = 505U,     ///< HTTP Version Not Supported.
    };

public:
    /// Create a known status value.
    constexpr HttpStatus(const Value value) noexcept : _code{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr HttpStatus() noexcept = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_code, const HttpStatus &other, other._code);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_code, const Value value, static_cast<uint16_t>(value));
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(
        const Value value, const HttpStatus &other, static_cast<uint16_t>(value), other._code);

public: // tests/accessors
    /// Test whether the code is in the valid 100--599 range.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _code >= 100U && _code <= 599U; }
    /// Test whether this is an informational status.
    [[nodiscard]] constexpr auto isInformational() const noexcept -> bool { return _code >= 100U && _code <= 199U; }
    /// Test whether this is a successful status.
    [[nodiscard]] constexpr auto isSuccessful() const noexcept -> bool { return _code >= 200U && _code <= 299U; }
    /// Test whether this is a redirection status.
    [[nodiscard]] constexpr auto isRedirection() const noexcept -> bool { return _code >= 300U && _code <= 399U; }
    /// Test whether this is a client-error status.
    [[nodiscard]] constexpr auto isClientError() const noexcept -> bool { return _code >= 400U && _code <= 499U; }
    /// Test whether this is a server-error status.
    [[nodiscard]] constexpr auto isServerError() const noexcept -> bool { return _code >= 500U && _code <= 599U; }
    /// Get the numeric status code, or zero for an invalid status.
    [[nodiscard]] constexpr auto code() const noexcept -> uint16_t { return _code; }
    /// Get the conventional reason phrase, or empty text for an unknown code.
    [[nodiscard]] auto defaultReasonPhrase() const noexcept -> text::String;

public: // conversion/factories
    /// Convert the numeric status code to three decimal digits, or empty text if invalid.
    [[nodiscard]] auto toString() const noexcept -> text::String;
    /// Create a status from a numeric code, returning an invalid placeholder outside 100--599.
    [[nodiscard]] static constexpr auto fromCode(const uint16_t code) noexcept -> HttpStatus {
        auto result = HttpStatus{};
        if (code >= 100U && code <= 599U) {
            result._code = code;
        }
        return result;
    }
    /// Create a status from a numeric code.
    /// @throws err::ParameterError If the code is outside 100--599.
    [[nodiscard]] static auto fromCodeOrThrow(uint16_t code) -> HttpStatus;
    /// Parse exactly three decimal digits, returning an invalid placeholder on failure.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> HttpStatus;
    /// Parse exactly three decimal digits.
    /// @throws err::ParseError If the text is malformed or outside 100--599.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> HttpStatus;

private:
    uint16_t _code{}; ///< Numeric status code.
};

}
