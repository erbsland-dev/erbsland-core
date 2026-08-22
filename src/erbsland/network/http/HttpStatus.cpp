// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpStatus.hpp"

#include "../../err/ParameterError.hpp"
#include "../../err/ParseError.hpp"
#include "../../text/IntegerBase.hpp"
#include "../../text/IntegerParseOptions.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/CpLength.hpp"

namespace erbsland::network {

using namespace text;
using namespace text::literals;

auto HttpStatus::defaultReasonPhrase() const noexcept -> String {
    switch (_code) {
    case Continue:
        return "Continue"_el;
    case SwitchingProtocols:
        return "Switching Protocols"_el;
    case EarlyHints:
        return "Early Hints"_el;
    case Ok:
        return "OK"_el;
    case Created:
        return "Created"_el;
    case Accepted:
        return "Accepted"_el;
    case NonAuthoritativeInformation:
        return "Non-Authoritative Information"_el;
    case NoContent:
        return "No Content"_el;
    case ResetContent:
        return "Reset Content"_el;
    case PartialContent:
        return "Partial Content"_el;
    case MultipleChoices:
        return "Multiple Choices"_el;
    case MovedPermanently:
        return "Moved Permanently"_el;
    case Found:
        return "Found"_el;
    case SeeOther:
        return "See Other"_el;
    case NotModified:
        return "Not Modified"_el;
    case UseProxy:
        return "Use Proxy"_el;
    case TemporaryRedirect:
        return "Temporary Redirect"_el;
    case PermanentRedirect:
        return "Permanent Redirect"_el;
    case BadRequest:
        return "Bad Request"_el;
    case Unauthorized:
        return "Unauthorized"_el;
    case PaymentRequired:
        return "Payment Required"_el;
    case Forbidden:
        return "Forbidden"_el;
    case NotFound:
        return "Not Found"_el;
    case MethodNotAllowed:
        return "Method Not Allowed"_el;
    case NotAcceptable:
        return "Not Acceptable"_el;
    case ProxyAuthenticationRequired:
        return "Proxy Authentication Required"_el;
    case RequestTimeout:
        return "Request Timeout"_el;
    case Conflict:
        return "Conflict"_el;
    case Gone:
        return "Gone"_el;
    case LengthRequired:
        return "Length Required"_el;
    case PreconditionFailed:
        return "Precondition Failed"_el;
    case ContentTooLarge:
        return "Content Too Large"_el;
    case UriTooLong:
        return "URI Too Long"_el;
    case UnsupportedMediaType:
        return "Unsupported Media Type"_el;
    case RangeNotSatisfiable:
        return "Range Not Satisfiable"_el;
    case ExpectationFailed:
        return "Expectation Failed"_el;
    case MisdirectedRequest:
        return "Misdirected Request"_el;
    case UnprocessableContent:
        return "Unprocessable Content"_el;
    case UpgradeRequired:
        return "Upgrade Required"_el;
    case TooManyRequests:
        return "Too Many Requests"_el;
    case RequestHeaderFieldsTooLarge:
        return "Request Header Fields Too Large"_el;
    case UnavailableForLegalReasons:
        return "Unavailable For Legal Reasons"_el;
    case InternalServerError:
        return "Internal Server Error"_el;
    case NotImplemented:
        return "Not Implemented"_el;
    case BadGateway:
        return "Bad Gateway"_el;
    case ServiceUnavailable:
        return "Service Unavailable"_el;
    case GatewayTimeout:
        return "Gateway Timeout"_el;
    case HttpVersionNotSupported:
        return "HTTP Version Not Supported"_el;
    default:
        return {};
    }
}

auto HttpStatus::toString() const noexcept -> String {
    return isValid() ? String::fromInteger(_code) : String{};
}

auto HttpStatus::fromCodeOrThrow(const uint16_t code) -> HttpStatus {
    const auto result = fromCode(code);
    if (!result.isValid()) {
        throw err::ParameterError{"An HTTP status code must be in the range 100 through 599."_el, "code"_el};
    }
    return result;
}

auto HttpStatus::fromString(const String &text) noexcept -> HttpStatus {
    try {
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return {};
    }
}

auto HttpStatus::fromStringOrThrow(const String &text) -> HttpStatus {
    try {
        const auto options = IntegerParseOptions{}
                                 .setFixedBase(IntegerBase::Decimal)
                                 .setMinimumDigits(unit::CpLength{3U})
                                 .setMaximumDigits(unit::CpLength{3U});
        const auto result = fromCode(text.toIntegerOrThrow<uint16_t>(options));
        if (result.isValid()) {
            return result;
        }
    } catch (const err::ParseError &) {}
    throw err::ParseError{"An HTTP status must be exactly three digits from 100 through 599."_el};
}

}
