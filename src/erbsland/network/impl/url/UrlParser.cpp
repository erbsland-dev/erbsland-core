// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UrlParser.hpp"

#include "../../../err/Exception.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../err/ParseError.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../text/CharSet.hpp"
#include "../../../text/EncodingMode.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/NormalizationForm.hpp"
#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringDecoder.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringEncoding.hpp"
#include "../../../text/StringList.hpp"

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

auto UrlParser::parse() const -> std::shared_ptr<UrlData> {
    if (_text.length() > _options.maximumLength()) {
        throw err::ParseError{"The URL exceeds the configured length limit."_el};
    }
    if (!_text.isValidUtf8()) {
        throw err::ParseError{"The URL is not valid UTF-8."_el};
    }
    auto reader = StringCharReader{_text};
    auto schemeEditor = StringEditor{};
    const auto first = reader.read();
    if (!first.isAsciiLetter()) {
        throw err::ParseError{"A URL scheme must start with an ASCII letter."_el};
    }
    schemeEditor.append(first.toAsciiLowercase());
    while (true) {
        const auto character = reader.read();
        if (character == U':') {
            break;
        }
        if (!character.isAsciiAlphanumeric() && character != U'+' && character != U'-' && character != U'.') {
            throw err::ParseError{"The URL scheme contains an invalid character."_el, reader.position()};
        }
        schemeEditor.append(character.toAsciiLowercase());
    }
    auto result = std::make_shared<UrlData>();
    result->schemeText = String{schemeEditor};
    result->scheme = schemeFromText(result->schemeText);

    const auto requiresAuthority = result->scheme == UrlScheme::Http || result->scheme == UrlScheme::Https ||
        result->scheme == UrlScheme::Ftp || result->scheme == UrlScheme::Ftps;
    if (reader.peek() == U'/') {
        const auto saved = reader.save();
        reader.advance();
        if (reader.advanceIf(U'/')) {
            if (result->scheme == UrlScheme::Mailto) {
                throw err::ParseError{"A mailto URL must not contain a slash."_el, reader.position()};
            }
            result->hasAuthority = true;
            auto authority = StringEditor{};
            while (!reader.isAtEnd() && reader.peek() != U'/' && reader.peek() != U'?' && reader.peek() != U'#') {
                authority.append(reader.read());
            }
            result->authorityText = String{authority};
        } else {
            reader.restore(saved);
        }
    }
    if (requiresAuthority && !result->hasAuthority) {
        throw err::ParseError{"This URL scheme requires a double-slash authority."_el, reader.position()};
    }

    auto path = StringEditor{};
    while (!reader.isAtEnd() && reader.peek() != U'?' && reader.peek() != U'#') {
        path.append(reader.read());
    }
    auto query = StringEditor{};
    if (reader.advanceIf(U'?')) {
        result->hasQuery = true;
        while (!reader.isAtEnd() && reader.peek() != U'#') {
            query.append(reader.read());
        }
    }
    auto fragment = StringEditor{};
    if (reader.advanceIf(U'#')) {
        result->hasFragment = true;
        while (!reader.isAtEnd()) {
            fragment.append(reader.read());
        }
    }
    result->path = decodeComponent(String{path});
    result->query = decodeComponent(String{query});
    result->fragment = decodeComponent(String{fragment});

    switch (result->scheme) {
    case UrlScheme::Http:
    case UrlScheme::Https:
    case UrlScheme::Ftp:
    case UrlScheme::Ftps:
        parseKnownAuthority(*result, defaultPort(result->scheme));
        break;
    case UrlScheme::File:
        if (result->hasAuthority) {
            parseFileAuthority(*result);
        }
        break;
    case UrlScheme::Mailto:
        if (result->hasAuthority) {
            throw err::ParseError{"A mailto URL must not contain an authority."_el};
        }
        if (result->path.contains("/"_el)) {
            throw err::ParseError{"A mailto URL path must not contain a slash."_el};
        }
        break;
    case UrlScheme::Custom:
        if (result->hasAuthority) {
            parseCustomAuthority(*result);
        }
        break;
    case UrlScheme::Invalid:
        break;
    }
    return result;
}

auto UrlParser::network(const UrlScheme scheme, HostEndpoint endpoint, String path, String query, String fragment)
    -> std::shared_ptr<UrlData> {
    if (scheme != UrlScheme::Http && scheme != UrlScheme::Https && scheme != UrlScheme::Ftp &&
        scheme != UrlScheme::Ftps) {
        throw err::ParameterError{"The URL scheme is not a host network scheme."_el, "scheme"_el};
    }
    if (endpoint.port().isAutomatic()) {
        throw err::ParameterError{"A remote URL endpoint must not use port zero."_el, "endpoint"_el};
    }
    validateAuthorityPath(path);
    auto result = std::make_shared<UrlData>();
    result->scheme = scheme;
    result->schemeText = schemeText(scheme);
    result->endpoint = std::move(endpoint);
    result->authorityText = result->endpoint.toString();
    result->path = normalizedComponent(std::move(path));
    result->query = normalizedComponent(std::move(query));
    result->fragment = normalizedComponent(std::move(fragment));
    result->hasQuery = !result->query.isEmpty();
    result->hasFragment = !result->fragment.isEmpty();
    result->hasAuthority = true;
    result->authorityParsed = true;
    result->hasEndpoint = true;
    return result;
}

auto UrlParser::file(std::optional<Host> host, String path, String query, String fragment) -> std::shared_ptr<UrlData> {
    if (host.has_value()) {
        validateAuthorityPath(path);
    }
    auto result = std::make_shared<UrlData>();
    result->scheme = UrlScheme::File;
    result->schemeText = "file"_el;
    result->path = normalizedComponent(std::move(path));
    result->query = normalizedComponent(std::move(query));
    result->fragment = normalizedComponent(std::move(fragment));
    result->hasQuery = !result->query.isEmpty();
    result->hasFragment = !result->fragment.isEmpty();
    if (host.has_value()) {
        result->endpoint = HostEndpoint{*host, Port{}};
        result->authorityText = host->toString();
        result->hasAuthority = true;
        result->authorityParsed = true;
        result->hasEndpoint = true;
    }
    return result;
}

auto UrlParser::mailto(String address, String query, String fragment) -> std::shared_ptr<UrlData> {
    address = normalizedComponent(std::move(address));
    validateMailtoPath(address);
    auto result = std::make_shared<UrlData>();
    result->scheme = UrlScheme::Mailto;
    result->schemeText = "mailto"_el;
    result->path = std::move(address);
    result->query = normalizedComponent(std::move(query));
    result->fragment = normalizedComponent(std::move(fragment));
    result->hasQuery = !result->query.isEmpty();
    result->hasFragment = !result->fragment.isEmpty();
    return result;
}

auto UrlParser::custom(String scheme, std::optional<String> authority, String path, String query, String fragment)
    -> std::shared_ptr<UrlData> {
    validateSchemeText(scheme);
    scheme = scheme.transformed(Char::toAsciiLowercase);
    if (schemeFromText(scheme) != UrlScheme::Custom) {
        throw err::ParameterError{"A custom URL requires an unrecognized scheme name."_el, "scheme"_el};
    }
    if (authority.has_value()) {
        validateAuthorityText(*authority);
        validateAuthorityPath(path);
    }
    auto result = std::make_shared<UrlData>();
    result->scheme = UrlScheme::Custom;
    result->schemeText = std::move(scheme);
    result->path = normalizedComponent(std::move(path));
    result->query = normalizedComponent(std::move(query));
    result->fragment = normalizedComponent(std::move(fragment));
    result->hasQuery = !result->query.isEmpty();
    result->hasFragment = !result->fragment.isEmpty();
    if (authority.has_value()) {
        result->hasAuthority = true;
        result->authorityText = std::move(*authority);
        parseCustomAuthority(*result);
    }
    return result;
}

auto UrlParser::schemeFromText(const String &text) noexcept -> UrlScheme {
    if (text == "http"_el) {
        return UrlScheme::Http;
    }
    if (text == "https"_el) {
        return UrlScheme::Https;
    }
    if (text == "ftp"_el) {
        return UrlScheme::Ftp;
    }
    if (text == "ftps"_el) {
        return UrlScheme::Ftps;
    }
    if (text == "file"_el) {
        return UrlScheme::File;
    }
    if (text == "mailto"_el) {
        return UrlScheme::Mailto;
    }
    return UrlScheme::Custom;
}

auto UrlParser::schemeText(const UrlScheme scheme) -> String {
    switch (scheme) {
    case UrlScheme::Http:
        return "http"_el;
    case UrlScheme::Https:
        return "https"_el;
    case UrlScheme::Ftp:
        return "ftp"_el;
    case UrlScheme::Ftps:
        return "ftps"_el;
    case UrlScheme::File:
        return "file"_el;
    case UrlScheme::Mailto:
        return "mailto"_el;
    case UrlScheme::Invalid:
    case UrlScheme::Custom:
        return {};
    }
    return {};
}

auto UrlParser::defaultPort(const UrlScheme scheme) noexcept -> Port {
    switch (scheme) {
    case UrlScheme::Http:
        return Port{80U};
    case UrlScheme::Https:
        return Port{443U};
    case UrlScheme::Ftp:
        return Port{21U};
    case UrlScheme::Ftps:
        return Port{990U};
    default:
        return {};
    }
}

auto UrlParser::decodeComponent(const String &text) -> String {
    auto reader = StringCharReader{text};
    auto result = StringEditor{};
    auto encoded = mem::ByteBlockEditor{};
    while (!reader.isAtEnd()) {
        if (reader.peek() != U'%') {
            if (!encoded.length().isZero()) {
                try {
                    result.append(
                        StringDecoder{encoded}.decode(
                            StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict));
                } catch (const err::Exception &) {
                    throw err::ParseError{"A percent-encoded URL component is not valid UTF-8."_el, reader.position()};
                }
                encoded.clear();
            }
            result.append(reader.read());
            continue;
        }
        reader.advance();
        const auto high = reader.read().digitValue(IntegerBase::Hexadecimal);
        const auto low = reader.read().digitValue(IntegerBase::Hexadecimal);
        if (!high.has_value() || !low.has_value()) {
            throw err::ParseError{"A URL percent escape requires two hexadecimal digits."_el, reader.position()};
        }
        encoded.append(mem::Byte{static_cast<uint8_t>((*high << 4U) | *low)});
    }
    if (!encoded.length().isZero()) {
        try {
            result.append(
                StringDecoder{encoded}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict));
        } catch (const err::Exception &) {
            throw err::ParseError{"A percent-encoded URL component is not valid UTF-8."_el};
        }
    }
    return String{result}.normalized(NormalizationForm::Nfc);
}

auto UrlParser::normalizedComponent(String text) -> String {
    if (!text.isValidUtf8()) {
        throw err::ParameterError{"A URL component must be valid UTF-8."_el, "text"_el};
    }
    return text.normalized(NormalizationForm::Nfc);
}

void UrlParser::parseKnownAuthority(UrlData &data, const Port port) {
    auto hostPort = data.authorityText;
    parseUserInfo(data, hostPort);
    data.endpoint = parseHostPort(hostPort, port, false);
    if (data.endpoint.port().isAutomatic()) {
        throw err::ParseError{"A remote URL endpoint must not use port zero."_el};
    }
    data.authorityParsed = true;
    data.hasEndpoint = true;
}

void UrlParser::parseFileAuthority(UrlData &data) {
    if (data.authorityText.isEmpty()) {
        data.authorityParsed = true;
        return;
    }
    if (data.authorityText.containsOneOf(CharSet{U'@', U':', U'[', U']'})) {
        throw err::ParseError{"A file URL authority must be a plain host without credentials or a port."_el};
    }
    data.endpoint = HostEndpoint{Host::fromStringOrThrow(decodeComponent(data.authorityText)), Port{}};
    data.authorityParsed = true;
    data.hasEndpoint = true;
}

void UrlParser::parseCustomAuthority(UrlData &data) noexcept {
    try {
        auto hostPort = data.authorityText;
        parseUserInfo(data, hostPort);
        data.endpoint = parseHostPort(hostPort, Port{}, true);
        if (data.endpoint.port().isAutomatic()) {
            return;
        }
        data.authorityParsed = true;
        data.hasEndpoint = true;
    } catch (const err::Exception &) {
        data.username = {};
        data.password = {};
        data.hasUserInfo = false;
        data.hasPassword = false;
        data.authorityParsed = false;
        data.hasEndpoint = false;
        data.endpoint = {};
    }
}

void UrlParser::parseUserInfo(UrlData &data, String &hostPort) {
    const auto parts = StringList::fromSplit(hostPort, CharSet{U'@'}, unit::ItemCount::infinite(), true);
    if (parts.count() > unit::ItemCount{2U}) {
        throw err::ParseError{"A URL authority contains more than one user-info delimiter."_el};
    }
    if (parts.count() == unit::ItemCount::one()) {
        return;
    }
    data.hasUserInfo = true;
    const auto userParts =
        StringList::fromSplit(parts.get(unit::ItemIndex::zero()), CharSet{U':'}, unit::ItemCount{1U}, true);
    data.username = decodeComponent(userParts.get(unit::ItemIndex::zero()));
    if (userParts.count() == unit::ItemCount{2U}) {
        data.hasPassword = true;
        data.password = decodeComponent(userParts.get(unit::ItemIndex{1U}));
    }
    hostPort = parts.get(unit::ItemIndex{1U});
}

auto UrlParser::parseHostPort(const String &text, const Port defaultPortValue, const bool requirePort) -> HostEndpoint {
    if (text.isEmpty()) {
        throw err::ParseError{"A URL authority requires a host."_el};
    }
    auto endpointText = text;
    auto hasPort = false;
    if (text.startsWith("["_el)) {
        const auto close = text.find("]"_el);
        if (close.isNoIndex()) {
            throw err::ParseError{"A bracketed URL host requires a closing bracket."_el};
        }
        const auto after = text.slice(StringSide::Back, close.advanced(unit::ByteLength::one()));
        if (after.isEmpty()) {
            // Add the scheme default below.
        } else if (after.startsWith(":"_el)) {
            hasPort = true;
        } else {
            throw err::ParseError{"Unexpected text follows a bracketed URL host."_el};
        }
        const auto percentCount = endpointText.count("%"_el);
        if (percentCount > unit::ItemCount::one() ||
            (percentCount == unit::ItemCount::one() && !endpointText.contains("%25"_el))) {
            throw err::ParseError{"An IPv6 URL scope delimiter must be percent-encoded as %25."_el};
        }
        if (percentCount == unit::ItemCount::one()) {
            endpointText = endpointText.replacedFirst("%25"_el, "%"_el);
        }
    } else {
        const auto parts = StringList::fromSplit(text, CharSet{U':'}, unit::ItemCount::infinite(), true);
        if (parts.count() > unit::ItemCount{2U}) {
            throw err::ParseError{"An IPv6 URL host must be enclosed in brackets."_el};
        }
        hasPort = parts.count() == unit::ItemCount{2U};
        if (!hasPort && (requirePort || defaultPortValue.isAutomatic())) {
            throw err::ParseError{"This URL authority requires an explicit port."_el};
        }
        const auto port = hasPort ? Port::fromStringOrThrow(parts.get(unit::ItemIndex{1U})) : defaultPortValue;
        return HostEndpoint{Host::fromStringOrThrow(decodeComponent(parts.get(unit::ItemIndex::zero()))), port};
    }
    if (!hasPort) {
        if (requirePort || defaultPortValue.isAutomatic()) {
            throw err::ParseError{"This URL authority requires an explicit port."_el};
        }
        auto editor = StringEditor{endpointText};
        editor.append(U':').append(defaultPortValue.toString());
        endpointText = String{editor};
    }
    return HostEndpoint::fromStringOrThrow(endpointText);
}

void UrlParser::validateSchemeText(const String &scheme) {
    if (!scheme.isValidUtf8() || scheme.isEmpty()) {
        throw err::ParameterError{"A custom URL scheme must not be empty."_el, "scheme"_el};
    }
    auto reader = StringCharReader{scheme};
    if (!reader.read().isAsciiLetter()) {
        throw err::ParameterError{"A custom URL scheme must start with an ASCII letter."_el, "scheme"_el};
    }
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!character.isAsciiAlphanumeric() && character != U'+' && character != U'-' && character != U'.') {
            throw err::ParameterError{"A custom URL scheme contains an invalid character."_el, "scheme"_el};
        }
    }
}

void UrlParser::validateMailtoPath(const String &path) {
    if (path.contains("/"_el)) {
        throw err::ParameterError{"A mailto URL path must not contain a slash."_el, "address"_el};
    }
}

void UrlParser::validateAuthorityPath(const String &path) {
    if (!path.isEmpty() && !path.startsWith("/"_el)) {
        throw err::ParameterError{"A URL path following an authority must start with a slash."_el, "path"_el};
    }
}

void UrlParser::validateAuthorityText(const String &authority) {
    if (!authority.isValidUtf8()) {
        throw err::ParameterError{"A custom URL authority must be valid UTF-8."_el, "authority"_el};
    }
    if (authority.containsOneOf(CharSet{U'/', U'?', U'#'})) {
        throw err::ParameterError{
            "A custom URL authority must not contain path, query, or fragment delimiters."_el, "authority"_el};
    }
}

}
