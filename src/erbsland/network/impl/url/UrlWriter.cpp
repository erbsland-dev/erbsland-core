// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UrlWriter.hpp"

#include "../../../text/EscapeFormat.hpp"
#include "../../../text/Literals.hpp"
#include "../../../unit/ByteCount.hpp"

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

auto UrlWriter::write() -> String {
    if (_data.scheme == UrlScheme::Invalid) {
        return {};
    }
    auto estimatedLength = unit::ByteCount{16U};
    estimatedLength += _data.schemeText.length();
    estimatedLength += _data.authorityText.length();
    estimatedLength += _data.username.length();
    estimatedLength += _data.password.length();
    estimatedLength += _data.path.length();
    estimatedLength += _data.query.length();
    estimatedLength += _data.fragment.length();
    _result.reserve(estimatedLength * 3U);

    _result.append(_data.schemeText).append(U':');
    if (_data.hasAuthority) {
        _result.append("//"_el);
        writeAuthority();
    }
    _result.append(_data.path.toEscaped(EscapeFormat::UrlPath));
    if (_data.hasQuery) {
        _result.append(U'?');
        _result.append(_data.query.toEscaped(EscapeFormat::UrlQuery));
    }
    if (_options.includesFragment() && _data.hasFragment) {
        _result.append(U'#');
        _result.append(_data.fragment.toEscaped(EscapeFormat::UrlQuery));
    }
    return _result;
}

auto UrlWriter::writeRequestTarget() -> String {
    if (_data.scheme != UrlScheme::Http && _data.scheme != UrlScheme::Https) {
        return {};
    }
    if (_data.path.isEmpty()) {
        _result.append(U'/');
    } else {
        _result.append(_data.path.toEscaped(EscapeFormat::UrlPath));
    }
    if (_data.hasQuery) {
        _result.append(U'?');
        _result.append(_data.query.toEscaped(EscapeFormat::UrlQuery));
    }
    return _result;
}

auto UrlWriter::writeHostField() -> String {
    if ((_data.scheme != UrlScheme::Http && _data.scheme != UrlScheme::Https) || !_data.hasEndpoint) {
        return {};
    }
    writeHost();
    if (_data.endpoint.port() != defaultPort()) {
        _result.append(U':').append(_data.endpoint.port().toString());
    }
    return _result;
}

void UrlWriter::writeAuthority() {
    if ((_data.scheme == UrlScheme::Custom || _data.scheme == UrlScheme::File) && !_data.authorityParsed) {
        _result.append(_options.redactsPassword() ? "***"_el : _data.authorityText);
        return;
    }
    writeParsedAuthority();
}

void UrlWriter::writeParsedAuthority() {
    if (_data.hasUserInfo) {
        _result.append(_data.username.toEscaped(EscapeFormat::UrlUserInfo));
        if (_data.hasPassword) {
            _result.append(U':');
            if (_options.redactsPassword()) {
                _result.append("***"_el);
            } else {
                _result.append(_data.password.toEscaped(EscapeFormat::UrlUserInfo));
            }
        }
        _result.append(U'@');
    }
    if (!_data.hasEndpoint) {
        return;
    }
    writeHost();
    if (_data.scheme != UrlScheme::File && (_options.includesDefaultPort() || _data.endpoint.port() != defaultPort())) {
        _result.append(U':').append(_data.endpoint.port().toString());
    }
}

void UrlWriter::writeHost() {
    const auto &host = _data.endpoint.host();
    if (const auto name = host.name(); name.has_value()) {
        _result.append(name->toString(_options.hostNameFormat()));
        return;
    }
    const auto address = host.address();
    if (address.has_value() && address->isV6()) {
        _result.append(U'[').append(host.toString());
        if (_data.endpoint.scopeId().isSpecified()) {
            _result.append("%25"_el).append(String::fromInteger(_data.endpoint.scopeId().toRawValue()));
        }
        _result.append(U']');
        return;
    }
    _result.append(host.toString());
}

auto UrlWriter::defaultPort() const noexcept -> Port {
    switch (_data.scheme) {
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

}
