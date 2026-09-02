// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsConfigurationParser.hpp"

#include "TlsServerIdentity.hpp"

#include "../impl/TlsConfigurationLabel.hpp"
#include "../impl/TlsConfigurationRules.hpp"
#include "../keys/SigningPrivateKey.hpp"
#include "../x509/X509CertificateBundle.hpp"
#include "../x509/X509ServerCertificatePolicy.hpp"

#include "../../conf/ConfError.hpp"
#include "../../conf/ConfErrorCategory.hpp"
#include "../../conf/ConfErrorContext.hpp"
#include "../../conf/SourceIdentifier.hpp"
#include "../../conf/Value.hpp"
#include "../../conf/vr/Rules.hpp"
#include "../../err/Exception.hpp"
#include "../../err/ParameterError.hpp"
#include "../../path/Path.hpp"
#include "../../path/PathResolveMode.hpp"
#include "../../text/Literals.hpp"

#include <exception>
#include <utility>

namespace erbsland::cryptology {

using namespace text::literals;

auto TlsConfigurationParser::validationRules() -> const conf::vr::RulesPtr & {
    static const auto cRules = impl::createTlsConfigurationRules();
    return cRules;
}

auto TlsConfigurationParser::version() -> conf::Integer {
    return conf::Integer{1};
}

auto TlsConfigurationParser::parse(const conf::ValuePtr &sectionValue) const -> TlsConfigurationEntry {
    if (sectionValue == nullptr) {
        throw err::ParameterError{"A TLS configuration section must not be empty."_el, "sectionValue"_el};
    }
    validationRules()->validate(sectionValue, version());
    const auto labelValue = sectionValue->valueOrThrow("label"_el);
    const auto label = labelValue->asText();
    try {
        impl::tls_configuration_label::validate(label);
    } catch (const err::Exception &) {
        throw conf::ConfError{
            conf::ConfErrorContext{
                conf::ConfErrorCategory::Validation,
                "Invalid TLS configuration label."_el,
                labelValue->namePath(),
                labelValue->location()},
            std::current_exception()};
    }

    auto configuration = TlsConfiguration{};
    const auto trustAnchorsValue = sectionValue->value("trust_anchors_file"_el);
    if (trustAnchorsValue != nullptr) {
        const auto intermediatesValue = sectionValue->value("intermediate_certificates_file"_el);
        auto trustAnchors = loadCertificateBundle(trustAnchorsValue, "Failed to load TLS trust anchors."_el);
        auto intermediates = intermediatesValue != nullptr
            ? loadCertificateBundle(intermediatesValue, "Failed to load TLS intermediate certificates."_el)
            : X509CertificateBundle{};
        configuration.setServerCertificatePolicy(
            X509ServerCertificatePolicy{std::move(trustAnchors), std::move(intermediates)});
    }

    const auto certificateChainValue = sectionValue->value("server_certificate_chain_file"_el);
    if (certificateChainValue != nullptr) {
        const auto signingKeyValue = sectionValue->valueOrThrow("server_signing_key_file"_el);
        auto certificateChain =
            loadCertificateBundle(certificateChainValue, "Failed to load the TLS server certificate chain."_el);
        auto signingKey = loadSigningPrivateKey(signingKeyValue);
        try {
            configuration.setServerIdentity(TlsServerIdentity{std::move(certificateChain), std::move(signingKey)});
        } catch (const err::Exception &) {
            throw conf::ConfError{
                conf::ConfErrorContext{
                    conf::ConfErrorCategory::Validation,
                    "The TLS server certificate and signing key do not form a valid TLS 1.3 identity."_el,
                    signingKeyValue->namePath(),
                    signingKeyValue->location()},
                std::current_exception()};
        }
    }
    return TlsConfigurationEntry{label, std::move(configuration)};
}

auto TlsConfigurationParser::loadCertificateBundle(const conf::ValuePtr &value, const text::String &errorMessage)
    -> X509CertificateBundle {
    const auto filePath = resolveFile(value);
    try {
        return X509CertificateBundle::fromFileOrThrow(filePath);
    } catch (const err::Exception &) {
        throw conf::ConfError{
            conf::ConfErrorContext{
                conf::ConfErrorCategory::Validation, errorMessage, value->namePath(), value->location()},
            std::current_exception()};
    }
}

auto TlsConfigurationParser::loadSigningPrivateKey(const conf::ValuePtr &value) -> SigningPrivateKey {
    const auto filePath = resolveFile(value);
    try {
        return SigningPrivateKey::fromFileOrThrow(filePath);
    } catch (const err::Exception &) {
        throw conf::ConfError{
            conf::ConfErrorContext{
                conf::ConfErrorCategory::Validation,
                "Failed to load the unencrypted PKCS#8 TLS server signing key."_el,
                value->namePath(),
                value->location()},
            std::current_exception()};
    }
}

auto TlsConfigurationParser::resolveFile(const conf::ValuePtr &value) -> path::Path {
    auto result = path::Path{value->asText()};
    if (!result.isValid()) {
        throw conf::ConfError{
            conf::ConfErrorCategory::Validation,
            "The TLS material file path is invalid."_el,
            value->namePath(),
            value->location()};
    }
    if (result.isAbsolute()) {
        return result;
    }
    const auto sourceIdentifier = value->location().sourceIdentifier();
    if (sourceIdentifier == nullptr || sourceIdentifier->name() != "file"_el || sourceIdentifier->path().isEmpty()) {
        throw conf::ConfError{
            conf::ConfErrorCategory::Validation,
            "A relative TLS material file requires an ELCL file source."_el,
            value->namePath(),
            value->location()};
    }
    const auto sourcePath = path::Path{sourceIdentifier->path()};
    if (!sourcePath.isAbsolute() || sourcePath.parent().isEmpty()) {
        throw conf::ConfError{
            conf::ConfErrorCategory::Validation,
            "The ELCL file source has no valid absolute parent directory."_el,
            value->namePath(),
            value->location()};
    }
    result = (sourcePath.parent() / result).resolve(path::PathResolveMode::Lexical);
    if (!result.isValid() || !result.isAbsolute()) {
        throw conf::ConfError{
            conf::ConfErrorCategory::Validation,
            "The resolved TLS material file path is invalid."_el,
            value->namePath(),
            value->location()};
    }
    return result;
}

}
