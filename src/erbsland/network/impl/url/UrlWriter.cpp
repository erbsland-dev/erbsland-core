// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UrlWriter.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringEncoder.hpp"
#include "../../../text/StringEncoding.hpp"
#include "../../../unit/ByteLength.hpp"

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

auto UrlWriter::write() -> String {
    if (_data.scheme == UrlScheme::Invalid) {
        return {};
    }
    auto estimatedLength = unit::ByteLength{16U};
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
    writeEncoded(_data.path, Component::Path);
    if (_data.hasQuery) {
        _result.append(U'?');
        writeEncoded(_data.query, Component::Query);
    }
    if (_options.includesFragment() && _data.hasFragment) {
        _result.append(U'#');
        writeEncoded(_data.fragment, Component::Fragment);
    }
    return String{_result};
}

auto UrlWriter::writeRequestTarget() -> String {
    if (_data.scheme != UrlScheme::Http && _data.scheme != UrlScheme::Https) {
        return {};
    }
    if (_data.path.isEmpty()) {
        _result.append(U'/');
    } else {
        writeEncoded(_data.path, Component::Path);
    }
    if (_data.hasQuery) {
        _result.append(U'?');
        writeEncoded(_data.query, Component::Query);
    }
    return String{_result};
}

auto UrlWriter::writeHostField() -> String {
    if ((_data.scheme != UrlScheme::Http && _data.scheme != UrlScheme::Https) || !_data.hasEndpoint) {
        return {};
    }
    writeHost();
    if (_data.endpoint.port() != defaultPort()) {
        _result.append(U':').append(_data.endpoint.port().toString());
    }
    return String{_result};
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
        writeEncoded(_data.username, Component::UserInfo);
        if (_data.hasPassword) {
            _result.append(U':');
            if (_options.redactsPassword()) {
                _result.append("***"_el);
            } else {
                writeEncoded(_data.password, Component::UserInfo);
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

void UrlWriter::writeEncoded(const String &value, const Component component) {
    auto reader = StringCharReader{value};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (isAllowed(character, component)) {
            _result.append(character);
            continue;
        }
        auto characterText = StringEditor{};
        characterText.append(character);
        const auto bytes = StringEncoder{characterText}.encode(StringEncoding::Utf8, StringBomMode::Reject);
        auto index = unit::ByteIndex::zero();
        while (index.isWithin(bytes.length())) {
            writeEncodedByte(bytes.get(index).toUInt8());
            ++index;
        }
    }
}

void UrlWriter::writeEncodedByte(const uint8_t value) {
    constexpr char32_t digits[]{
        U'0', U'1', U'2', U'3', U'4', U'5', U'6', U'7', U'8', U'9', U'A', U'B', U'C', U'D', U'E', U'F'};
    _result.append(U'%').append(digits[(value >> 4U) & 0x0FU]).append(digits[value & 0x0FU]);
}

auto UrlWriter::isAllowed(const Char character, const Component component) const noexcept -> bool {
    if (!character.isAscii()) {
        return false;
    }
    if (character.isAsciiAlphanumeric() || character == U'-' || character == U'.' || character == U'_' ||
        character == U'~') {
        return true;
    }
    const auto isSubDelimiter = character == U'!' || character == U'$' || character == U'&' || character == U'\'' ||
        character == U'(' || character == U')' || character == U'*' || character == U'+' || character == U',' ||
        character == U';' || character == U'=';
    if (isSubDelimiter) {
        return true;
    }
    if (component == Component::UserInfo) {
        return false;
    }
    if (character == U':' || character == U'@' || character == U'/') {
        return true;
    }
    return (component == Component::Query || component == Component::Fragment) && character == U'?';
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
