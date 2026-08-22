// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerProtocolOptions.hpp"

#include "../TlsAlpnProtocol.hpp"

#include "../../../../cryptology/tls/TlsServerIdentity.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../text/Literals.hpp"

#include <set>

namespace erbsland::network::impl {

using namespace text::literals;

TlsServerProtocolOptions::TlsServerProtocolOptions(
    cryptology::TlsServerIdentityConstPtr identity,
    std::vector<text::String> alpnProtocols,
    std::vector<cryptology::TlsCipherSuite> cipherSuites,
    const SocketBufferLimits bufferLimits) :
    _defaultIdentity{std::move(identity)},
    _alpnProtocols{std::move(alpnProtocols)},
    _cipherSuites{std::move(cipherSuites)},
    _bufferLimits{bufferLimits} {
    if (_defaultIdentity == nullptr) {
        throw err::ParameterError{"TLS server protocol options require a server identity."_el, "identity"_el};
    }
    if (_cipherSuites.empty()) {
        throw err::ParameterError{
            "TLS server protocol options require at least one cipher suite."_el, "cipherSuites"_el};
    }
    auto suiteValues = std::set<uint16_t>{};
    for (const auto suite : _cipherSuites) {
        if (!cryptology::TlsCipherSuite::fromRawValue(suite.toRawValue()).has_value() ||
            !suiteValues.insert(suite.toRawValue()).second) {
            throw err::ParameterError{"TLS server cipher suites must be supported and unique."_el, "cipherSuites"_el};
        }
    }

    if (_alpnProtocols.size() > 64U) {
        throw err::ParameterError{"TLS server ALPN configuration exceeds 64 protocols."_el, "alpnProtocols"_el};
    }
    auto encodedAlpnLength = std::size_t{0U};
    auto uniqueAlpn = std::set<text::String, TlsAlpnProtocol::Less>{};
    for (const auto &protocol : _alpnProtocols) {
        const auto length = TlsAlpnProtocol::bytes(protocol).size();
        if (length == 0U || length > 255U || !uniqueAlpn.insert(protocol).second) {
            throw err::ParameterError{
                "TLS server ALPN identifiers must be unique and contain between 1 and 255 bytes."_el,
                "alpnProtocols"_el};
        }
        encodedAlpnLength += 1U + length;
    }
    if (encodedAlpnLength > 4096U) {
        throw err::ParameterError{"TLS server ALPN configuration exceeds 4096 encoded bytes."_el, "alpnProtocols"_el};
    }

    validateIdentity(_defaultIdentity);
}

auto TlsServerProtocolOptions::withNamedIdentities(
    cryptology::TlsServerIdentityConstPtr defaultIdentity,
    std::vector<NamedIdentity> namedIdentities,
    std::vector<text::String> alpnProtocols,
    std::vector<cryptology::TlsCipherSuite> cipherSuites,
    const SocketBufferLimits bufferLimits) -> TlsServerProtocolOptions {
    if (namedIdentities.size() > 64U) {
        throw err::ParameterError{"TLS server protocol options exceed 64 named identities."_el, "namedIdentities"_el};
    }
    auto serverNames = std::set<HostName>{};
    for (const auto &namedIdentity : namedIdentities) {
        if (!serverNames.insert(namedIdentity.serverName).second) {
            throw err::ParameterError{"TLS server names must be unique."_el, "namedIdentities"_el};
        }
        validateIdentity(namedIdentity.identity);
    }
    auto result = TlsServerProtocolOptions{
        std::move(defaultIdentity), std::move(alpnProtocols), std::move(cipherSuites), bufferLimits};
    result._namedIdentities = std::move(namedIdentities);
    return result;
}

void TlsServerProtocolOptions::validateIdentity(const cryptology::TlsServerIdentityConstPtr &identity) {
    if (identity == nullptr) {
        throw err::ParameterError{"TLS server identities must not be empty."_el, "identity"_el};
    }
    const auto &certificates = identity->certificateChain().certificates();
    if (certificates.count().toSizeT() > 16U) {
        throw err::ParameterError{"TLS server identity exceeds the 16-certificate protocol bound."_el, "identity"_el};
    }
    auto certificateBodyLength = std::size_t{1U + 3U};
    for (const auto &certificate : certificates) {
        certificateBodyLength += 3U + certificate.toDer().length().toSizeT() + 2U;
    }
    if (certificateBodyLength > 1024U * 1024U) {
        throw err::ParameterError{"TLS server Certificate message exceeds one MiB."_el, "identity"_el};
    }
}

auto TlsServerProtocolOptions::defaultCipherSuites() -> std::vector<cryptology::TlsCipherSuite> {
    const auto suites = cryptology::TlsCipherSuite::all();
    return {suites.begin(), suites.end()};
}

}
