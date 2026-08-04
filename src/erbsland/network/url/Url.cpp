// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Url.hpp"

#include "../impl/UrlData.hpp"
#include "../impl/UrlParser.hpp"
#include "../impl/UrlWriter.hpp"

#include "../../err/ParseError.hpp"

namespace erbsland::network {

Url::Url(HostEndpoint endpoint, text::String path, text::String query, text::String fragment) :
    Url{UrlScheme::Https, std::move(endpoint), std::move(path), std::move(query), std::move(fragment)} {
}

Url::Url(const UrlScheme scheme, HostEndpoint endpoint, text::String path, text::String query, text::String fragment) :
    _data{
        impl::UrlParser::network(scheme, std::move(endpoint), std::move(path), std::move(query), std::move(fragment))} {
}

auto Url::isValid() const noexcept -> bool {
    return _data != nullptr && _data->scheme != UrlScheme::Invalid;
}
auto Url::scheme() const noexcept -> UrlScheme {
    return _data != nullptr ? _data->scheme : UrlScheme::Invalid;
}
auto Url::schemeText() const noexcept -> text::String {
    return _data != nullptr ? _data->schemeText : text::String{};
}
auto Url::isSecureScheme() const noexcept -> bool {
    return scheme() == UrlScheme::Https || scheme() == UrlScheme::Ftps;
}
auto Url::endpoint() const noexcept -> HostEndpoint {
    return _data != nullptr ? _data->endpoint : HostEndpoint{};
}
auto Url::authorityText() const noexcept -> text::String {
    return _data != nullptr ? _data->authorityText : text::String{};
}
auto Url::username() const noexcept -> text::String {
    return _data != nullptr ? _data->username : text::String{};
}
auto Url::password() const noexcept -> text::String {
    return _data != nullptr ? _data->password : text::String{};
}
auto Url::path() const noexcept -> text::String {
    return _data != nullptr ? _data->path : text::String{};
}
auto Url::query() const noexcept -> text::String {
    return _data != nullptr ? _data->query : text::String{};
}
auto Url::fragment() const noexcept -> text::String {
    return _data != nullptr ? _data->fragment : text::String{};
}

auto Url::toString(const UrlFormatOptions options) const -> text::String {
    return isValid() ? impl::UrlWriter{*_data, options}.write() : text::String{};
}

auto Url::fromString(const text::String &text, const UrlParseOptions options) noexcept -> Url {
    try {
        return fromStringOrThrow(text, options);
    } catch (const err::ParseError &) {
        return {};
    }
}

auto Url::fromStringOrThrow(const text::String &text, const UrlParseOptions options) -> Url {
    return Url{impl::UrlParser{text, options}.parse()};
}

auto Url::file(text::String path, text::String query, text::String fragment) -> Url {
    return Url{impl::UrlParser::file({}, std::move(path), std::move(query), std::move(fragment))};
}
auto Url::file(Host host, text::String path, text::String query, text::String fragment) -> Url {
    return Url{impl::UrlParser::file(std::move(host), std::move(path), std::move(query), std::move(fragment))};
}
auto Url::mailto(text::String address, text::String query, text::String fragment) -> Url {
    return Url{impl::UrlParser::mailto(std::move(address), std::move(query), std::move(fragment))};
}
auto Url::custom(text::String scheme, text::String path, text::String query, text::String fragment) -> Url {
    return Url{impl::UrlParser::custom(std::move(scheme), {}, std::move(path), std::move(query), std::move(fragment))};
}
auto Url::customWithAuthority(
    text::String scheme, text::String authority, text::String path, text::String query, text::String fragment) -> Url {
    return Url{impl::UrlParser::custom(
        std::move(scheme), std::move(authority), std::move(path), std::move(query), std::move(fragment))};
}

}
