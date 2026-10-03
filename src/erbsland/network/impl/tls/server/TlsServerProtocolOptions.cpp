// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsServerProtocolOptions.hpp"

#include "../TlsAlpnProtocol.hpp"
#include "../TlsKeyShare.hpp"

#include "../../../../cryptology/tls/TlsIdentity.hpp"
#include "../../../../err/ParameterFailure.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../../unit/ItemCount.hpp"
#include "../../../../util/Set.hpp"

#include <set>

namespace erbsland::network::impl {

using namespace err;
using namespace text::literals;

TlsServerProtocolOptions::TlsServerProtocolOptions(
    TlsServerProtocolConfiguration configuration,
    text::StringList alpnProtocols,
    std::vector<cryptology::TlsCipherSuite> cipherSuites,
    const SocketBufferLimits bufferLimits,
    std::vector<cryptology::KeyAgreementAlgorithm> supportedGroups) :
    _defaultConfiguration{std::move(configuration)},
    _alpnProtocols{std::move(alpnProtocols)},
    _cipherSuites{std::move(cipherSuites)},
    _supportedGroups{std::move(supportedGroups)},
    _bufferLimits{bufferLimits} {
    tls_key_share::validate(_supportedGroups);
    if (_defaultConfiguration.identity == nullptr) {
        throw ParameterFailure{"TLS server protocol options require a server identity."_el, "identity"_el};
    }
    if (_cipherSuites.empty()) {
        throw ParameterFailure{"TLS server protocol options require at least one cipher suite."_el, "cipherSuites"_el};
    }
    auto suiteValues = std::set<uint16_t>{};
    for (const auto suite : _cipherSuites) {
        if (!cryptology::TlsCipherSuite::fromRawValue(suite.toRawValue()).has_value() ||
            !suiteValues.insert(suite.toRawValue()).second) {
            throw ParameterFailure{"TLS server cipher suites must be supported and unique."_el, "cipherSuites"_el};
        }
    }

    if (_alpnProtocols.count() > unit::ItemCount{64U}) {
        throw ParameterFailure{"TLS server ALPN configuration exceeds 64 protocols."_el, "alpnProtocols"_el};
    }
    auto encodedAlpnLength = std::size_t{0U};
    auto uniqueAlpn = util::Set<text::String, tls_alpn_protocol::Less>{};
    for (const auto &protocol : _alpnProtocols) {
        const auto length = tls_alpn_protocol::bytes(protocol).size();
        if (length == 0U || length > 255U || !uniqueAlpn.tryInsert(protocol)) {
            throw ParameterFailure{
                "TLS server ALPN identifiers must be unique and contain between 1 and 255 bytes."_el,
                "alpnProtocols"_el};
        }
        encodedAlpnLength += 1U + length;
    }
    if (encodedAlpnLength > 4096U) {
        throw ParameterFailure{"TLS server ALPN configuration exceeds 4096 encoded bytes."_el, "alpnProtocols"_el};
    }

    validateConfiguration(_defaultConfiguration);
}

auto TlsServerProtocolOptions::withNamedConfigurations(
    TlsServerProtocolConfiguration defaultConfiguration,
    std::vector<NamedConfiguration> namedConfigurations,
    text::StringList alpnProtocols,
    std::vector<cryptology::TlsCipherSuite> cipherSuites,
    const SocketBufferLimits bufferLimits,
    std::vector<cryptology::KeyAgreementAlgorithm> supportedGroups) -> TlsServerProtocolOptions {
    if (namedConfigurations.size() > 64U) {
        throw ParameterFailure{
            "TLS server protocol options exceed 64 named configurations."_el, "namedConfigurations"_el};
    }
    auto serverNames = std::set<HostName>{};
    for (const auto &namedIdentity : namedConfigurations) {
        if (!serverNames.insert(namedIdentity.serverName).second) {
            throw ParameterFailure{"TLS server names must be unique."_el, "namedConfigurations"_el};
        }
        validateConfiguration(namedIdentity.configuration);
    }
    auto result = TlsServerProtocolOptions{
        std::move(defaultConfiguration),
        std::move(alpnProtocols),
        std::move(cipherSuites),
        bufferLimits,
        std::move(supportedGroups)};
    result._namedConfigurations = std::move(namedConfigurations);
    return result;
}

void TlsServerProtocolOptions::validateConfiguration(const TlsServerProtocolConfiguration &configuration) {
    const auto mode = configuration.clientAuthentication;
    if (mode != cryptology::TlsClientAuthentication::Disabled &&
        mode != cryptology::TlsClientAuthentication::Optional &&
        mode != cryptology::TlsClientAuthentication::Required) {
        throw ParameterFailure{"Invalid TLS client authentication mode."_el, "configuration"_el};
    }
    if (mode != cryptology::TlsClientAuthentication::Disabled && !configuration.certificatePolicy) {
        throw ParameterFailure{"TLS client authentication requires a certificate policy."_el, "configuration"_el};
    }
    if (configuration.identity == nullptr) {
        throw ParameterFailure{"TLS server identities must not be empty."_el, "identity"_el};
    }
}

auto TlsServerProtocolOptions::defaultCipherSuites() -> std::vector<cryptology::TlsCipherSuite> {
    const auto suites = cryptology::TlsCipherSuite::all();
    return {suites.begin(), suites.end()};
}

}
