// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Url.hpp"

#include "../impl/url/UrlData.hpp"
#include "../impl/url/UrlParser.hpp"
#include "../impl/url/UrlResolver.hpp"
#include "../impl/url/UrlWriter.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text;
using namespace text::literals;

Url::Url(HostEndpoint endpoint, String path, String query, String fragment) :
    Url{UrlScheme::Https, std::move(endpoint), std::move(path), std::move(query), std::move(fragment)} {
}

Url::Url(const UrlScheme scheme, HostEndpoint endpoint, String path, String query, String fragment) :
    _data{
        impl::UrlParser::network(scheme, std::move(endpoint), std::move(path), std::move(query), std::move(fragment))} {
}

auto Url::isValid() const noexcept -> bool {
    return _data != nullptr && _data->scheme != UrlScheme::Invalid;
}

auto Url::scheme() const noexcept -> UrlScheme {
    return _data != nullptr ? _data->scheme : UrlScheme::Invalid;
}

auto Url::schemeText() const noexcept -> String {
    return _data != nullptr ? _data->schemeText : String{};
}

auto Url::isSecureScheme() const noexcept -> bool {
    return scheme() == UrlScheme::Https || scheme() == UrlScheme::Ftps;
}

auto Url::endpoint() const noexcept -> HostEndpoint {
    return _data != nullptr ? _data->endpoint : HostEndpoint{};
}

auto Url::authorityText() const noexcept -> String {
    return _data != nullptr ? _data->authorityText : String{};
}

auto Url::username() const noexcept -> String {
    return _data != nullptr ? _data->username : String{};
}

auto Url::password() const noexcept -> String {
    return _data != nullptr ? _data->password : String{};
}

auto Url::path() const noexcept -> String {
    return _data != nullptr ? _data->path : String{};
}

auto Url::query() const noexcept -> String {
    return _data != nullptr ? _data->query : String{};
}

auto Url::hasQuery() const noexcept -> bool {
    return _data != nullptr && _data->hasQuery;
}

auto Url::fragment() const noexcept -> String {
    return _data != nullptr ? _data->fragment : String{};
}

auto Url::hasFragment() const noexcept -> bool {
    return _data != nullptr && _data->hasFragment;
}

auto Url::toString(const UrlFormatOptions options) const -> String {
    return isValid() ? impl::UrlWriter{*_data, options}.write() : String{};
}

auto Url::fromString(const String &text, const UrlParseOptions options) noexcept -> Url {
    try {
        return fromStringOrThrow(text, options);
    } catch (const err::ParseError &) {
        return {};
    }
}

auto Url::fromStringOrThrow(const String &text, const UrlParseOptions options) -> Url {
    return Url{impl::UrlParser{text, options}.parse()};
}

auto Url::resolved(const String &reference, const UrlParseOptions options) const noexcept -> Url {
    try {
        return resolvedOrThrow(reference, options);
    } catch (const err::ParseError &) {
        return {};
    }
}

auto Url::resolvedOrThrow(const String &reference, const UrlParseOptions options) const -> Url {
    if (!isValid()) {
        throw err::ParseError{"A relative URI reference requires a valid base URL."_el};
    }
    return Url{impl::UrlResolver{*_data, reference, options}.resolve()};
}

auto Url::file(String path, String query, String fragment) -> Url {
    return Url{impl::UrlParser::file({}, std::move(path), std::move(query), std::move(fragment))};
}

auto Url::file(Host host, String path, String query, String fragment) -> Url {
    return Url{impl::UrlParser::file(std::move(host), std::move(path), std::move(query), std::move(fragment))};
}

auto Url::mailto(String address, String query, String fragment) -> Url {
    return Url{impl::UrlParser::mailto(std::move(address), std::move(query), std::move(fragment))};
}

auto Url::custom(String scheme, String path, String query, String fragment) -> Url {
    return Url{impl::UrlParser::custom(std::move(scheme), {}, std::move(path), std::move(query), std::move(fragment))};
}

auto Url::customWithAuthority(String scheme, String authority, String path, String query, String fragment) -> Url {
    return Url{impl::UrlParser::custom(
        std::move(scheme), std::move(authority), std::move(path), std::move(query), std::move(fragment))};
}

}
