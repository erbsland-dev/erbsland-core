// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsConfigurationParser.hpp"

#include "SystemIdentityStore.hpp"
#include "TlsIdentity.hpp"

#include "../configuration/CryptologyConfiguration.hpp"
#include "../CryptologyError.hpp"
#include "../impl/TlsConfigurationLabel.hpp"
#include "../impl/TlsConfigurationRules.hpp"
#include "../keys/SigningPrivateKey.hpp"
#include "../x509/X509CertificateBundle.hpp"
#include "../x509/X509TlsCertificatePolicy.hpp"

#include "../../conf/ConfError.hpp"
#include "../../conf/ConfErrorCategory.hpp"
#include "../../conf/ConfErrorContext.hpp"
#include "../../conf/SourceIdentifier.hpp"
#include "../../conf/Value.hpp"
#include "../../conf/vr/Rules.hpp"
#include "../../err/Exception.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../path/Path.hpp"
#include "../../path/PathResolveMode.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringLiteral.hpp"

#include <exception>
#include <utility>

namespace erbsland::cryptology {

using namespace err;
using namespace path;
using namespace text::literals;

auto TlsConfigurationParser::validationRules() -> const conf::vr::RulesPtr & {
    static const auto cRules = impl::createTlsConfigurationRules();
    return cRules;
}

auto TlsConfigurationParser::version() -> conf::Integer {
    return conf::Integer{2};
}

auto TlsConfigurationParser::parse(const conf::ValuePtr &sectionValue) const -> TlsConfigurationSet {
    auto result = TlsConfigurationSet{};
    if (sectionValue == nullptr) {
        return result;
    }
    validationRules()->validate(sectionValue, version());
    auto settings = TlsSettings{};
    if (const auto value = sectionValue->value(cSystemTrustValue); value) {
        settings.setSystemTrust(value->asBoolean());
    }
    if (const auto value = sectionValue->value("trust_allow_network"_el); value) {
        settings.setTrustAllowNetwork(value->asBoolean());
    }
    if (const auto value = sectionValue->value("trust_bundle_file"_el); value) {
        settings.setTrustBundlePath(resolveFile(value));
    }
    if (const auto value = sectionValue->value("identity_allow_interaction"_el); value) {
        settings.setIdentityAllowInteraction(value->asBoolean());
    }
    result.setSettings(settings);
    auto profiles = text::StringHashMap<TlsConfiguration>{};
    for (const auto role : {TlsRole::Client, TlsRole::Server}) {
        auto roleName = text::String{"client"_el};
        if (role == TlsRole::Server) {
            roleName = "server"_el;
        }
        const auto sections = sectionValue->value(roleName);
        if (!sections) {
            continue;
        }
        for (const auto &section : *sections) {
            const auto labelValue = section->valueOrThrow("label"_el);
            const auto label = labelValue->asText();
            auto configuration = profiles.get(label).value_or(TlsConfiguration{});
            if (configuration.hasRole(role)) {
                throw conf::ConfError{labelValue, "Duplicate TLS profile label for this role."_el};
            }
            auto trust = TlsTrustConfiguration{};
            if (const auto value = section->value("trust_anchors_file"_el); value) {
                trust.setTrustAnchors(loadCertificateBundle(value, "Failed to load TLS trust anchors."_el));
            }
            if (const auto value = section->value("intermediate_certificates_file"_el); value) {
                trust.setIntermediates(
                    loadCertificateBundle(value, "Failed to load TLS intermediate certificates."_el));
            }
            if (const auto value = section->value(cSystemTrustValue); value) {
                trust.setSystemTrust(value->asBoolean());
            }
            auto identity = loadIdentity(section, settings.allowsIdentityInteraction());
            try {
                if (role == TlsRole::Client) {
                    [[maybe_unused]] const auto policy = trust.resolve(settings, role);
                    auto client = TlsClientConfiguration{};
                    client.setTrust(std::move(trust));
                    if (identity) {
                        client.setIdentity(std::move(*identity));
                    }
                    configuration.setClient(std::move(client));
                } else {
                    if (!identity) {
                        throw conf::ConfError{section, "A TLS server profile requires an identity."_el};
                    }
                    auto server = TlsServerConfiguration{};
                    server.setIdentity(std::move(*identity));
                    if (const auto value = section->value("client_authentication"_el); value) {
                        if (value->asText() == "optional"_el) {
                            server.setClientAuthentication(TlsClientAuthentication::Optional);
                        } else if (value->asText() == "required"_el) {
                            server.setClientAuthentication(TlsClientAuthentication::Required);
                        }
                    }
                    [[maybe_unused]] const auto policy = trust.resolve(
                        settings, role, server.clientAuthentication() != TlsClientAuthentication::Disabled);
                    server.setTrust(std::move(trust));
                    configuration.setServer(std::move(server));
                }
            } catch (const RuntimeError &) {
                throw conf::ConfError{
                    conf::ConfErrorContext{
                        conf::ConfErrorCategory::Validation,
                        "Invalid TLS profile trust or identity configuration."_el,
                        section->namePath(),
                        section->location()},
                    std::current_exception()};
            }
            profiles.set(label, std::move(configuration));
            if (profiles.count().toSizeT() > CryptologyConfiguration::cMaximumTlsConfigurations) {
                throw conf::ConfError{section, "The TLS profile registry exceeds 256 labels."_el};
            }
        }
    }
    for (const auto &[label, configuration] : profiles) {
        result.add(label, configuration);
    }
    return result;
}

auto TlsConfigurationParser::loadIdentity(const conf::ValuePtr &section, const bool allowInteraction)
    -> std::optional<TlsIdentity> {
    const auto certificate = section->value("certificate_chain_file"_el);
    const auto key = section->value("signing_key_file"_el);
    const auto password = section->value("signing_key_password"_el);
    const auto fingerprint = section->value("identity_fingerprint"_el);
    if (fingerprint != nullptr && (certificate != nullptr || key != nullptr || password != nullptr)) {
        throw conf::ConfError{fingerprint, "Native and file identity sources must not be combined."_el};
    }
    if (password != nullptr && key == nullptr) {
        throw conf::ConfError{password, "A signing key password requires a signing key file."_el};
    }
    if (fingerprint != nullptr) {
        try {
            return SystemIdentityStore{allowInteraction}.load(fingerprint->asBytes());
        } catch (const RuntimeError &) {
            throw conf::ConfError{
                conf::ConfErrorContext{
                    conf::ConfErrorCategory::Validation,
                    "The exact native TLS identity could not be loaded."_el,
                    fingerprint->namePath(),
                    fingerprint->location()},
                std::current_exception()};
        }
    }
    if (certificate == nullptr && key == nullptr) {
        return std::nullopt;
    }
    if (certificate == nullptr || key == nullptr) {
        throw conf::ConfError{section, "A TLS identity requires both certificate chain and signing key files."_el};
    }
    auto chain = loadCertificateBundle(certificate, "Failed to load the TLS identity certificate chain."_el);
    auto signingKey = loadSigningPrivateKey(key, password);
    try {
        return TlsIdentity{std::move(chain), std::move(signingKey)};
    } catch (const CryptologyError &) {
        throw conf::ConfError{
            conf::ConfErrorContext{
                conf::ConfErrorCategory::Validation,
                "The certificate and signing key do not form a valid TLS 1.3 identity."_el,
                key->namePath(),
                key->location()},
            std::current_exception()};
    }
}

auto TlsConfigurationParser::loadCertificateBundle(const conf::ValuePtr &value, const text::String &errorMessage)
    -> X509CertificateBundle {
    const auto filePath = resolveFile(value);
    try {
        return X509CertificateBundle::fromFileOrThrow(filePath);
    } catch (const RuntimeError &) {
        throw conf::ConfError{
            conf::ConfErrorContext{
                conf::ConfErrorCategory::Validation, errorMessage, value->namePath(), value->location()},
            std::current_exception()};
    }
}

auto TlsConfigurationParser::loadSigningPrivateKey(const conf::ValuePtr &value, const conf::ValuePtr &password)
    -> SigningPrivateKey {
    const auto filePath = resolveFile(value);
    try {
        if (password != nullptr) {
            // Presence selects encrypted input; an existing empty vault value is a real empty password, not absence.
            return SigningPrivateKey::fromEncryptedFileOrThrow(filePath, password->asText());
        }
        return SigningPrivateKey::fromFileOrThrow(filePath);
    } catch (const RuntimeError &) {
        throw conf::ConfError{
            conf::ConfErrorContext{
                conf::ConfErrorCategory::Validation,
                "Failed to load the PKCS#8 TLS signing key."_el,
                value->namePath(),
                value->location()},
            std::current_exception()};
    }
}

auto TlsConfigurationParser::resolveFile(const conf::ValuePtr &value) -> Path {
    auto result = Path{value->asText()};
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
    const auto sourcePath = Path{sourceIdentifier->path()};
    if (!sourcePath.isAbsolute() || sourcePath.parent().isEmpty()) {
        throw conf::ConfError{
            conf::ConfErrorCategory::Validation,
            "The ELCL file source has no valid absolute parent directory."_el,
            value->namePath(),
            value->location()};
    }
    result = (sourcePath.parent() / result).resolve(PathResolveMode::Lexical);
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
