// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsConfigurationRules.hpp"

#include "../configuration/CryptologyConfiguration.hpp"

#include "../../conf/vr/builder/all.hpp"
#include "../../conf/vr/DependencyMode.hpp"
#include "../../conf/vr/RulesBuilder.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

auto createTlsConfigurationRules() -> conf::vr::RulesPtr {
    using namespace conf::vr::builder;
    using conf::vr::RuleType;
    auto rb = conf::vr::RulesBuilder{};

    rb.configureRoot(
        Title("TLS Configuration"_el),
        Description("One complete labeled TLS configuration registry entry."_el),
        Dependency(
            conf::vr::DependencyMode::If,
            {"intermediate_certificates_file"_el},
            {"trust_anchors_file"_el},
            "Intermediate certificates require explicit trust anchors."_el),
        Dependency(
            conf::vr::DependencyMode::XNOR,
            {"server_certificate_chain_file"_el},
            {"server_signing_key_file"_el},
            "A server certificate chain and server signing key must be configured together."_el));

    rb.addRule(
        "label"_el,
        RuleType::Text,
        Title("TLS Configuration Label"_el),
        Description("The lowercase slash-delimited application registry label, or empty for the global default."_el),
        Maximum(CryptologyConfiguration::cMaximumTlsConfigurationLabelLength),
        Matches("^([a-z0-9]+(-[a-z0-9]+)*(/[a-z0-9]+(-[a-z0-9]+)*){0,15})?$"_el));
    rb.addRule(
        "trust_anchors_file"_el,
        RuleType::Text,
        Title("Trust Anchors File"_el),
        Description("A PEM certificate bundle or one DER certificate containing explicit trust anchors."_el),
        Minimum(1),
        IsOptional());
    rb.addRule(
        "intermediate_certificates_file"_el,
        RuleType::Text,
        Title("Intermediate Certificates File"_el),
        Description("A PEM certificate bundle or one DER certificate containing additional issuer candidates."_el),
        Minimum(1),
        IsOptional());
    rb.addRule(
        "server_certificate_chain_file"_el,
        RuleType::Text,
        Title("Server Certificate Chain File"_el),
        Description("A leaf-first PEM certificate chain or one DER server certificate."_el),
        Minimum(1),
        IsOptional());
    rb.addRule(
        "server_signing_key_file"_el,
        RuleType::Text,
        Title("Server Signing Key File"_el),
        Description("An unencrypted PKCS#8 PEM or DER private key matching the server leaf certificate."_el),
        Minimum(1),
        IsOptional());
    return rb.takeRules();
}

}
