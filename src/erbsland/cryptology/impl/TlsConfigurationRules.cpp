// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsConfigurationRules.hpp"

#include "../configuration/CryptologyConfiguration.hpp"

#include "../../conf/vr/all.hpp"
#include "../../conf/vr/builder/all.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace text;
using namespace text::literals;

auto createTlsConfigurationRules() -> conf::vr::RulesPtr {
    using namespace conf::vr::builder;
    using conf::vr::RuleType;
    constexpr auto cSystemTrust = "system_trust"_el;
    auto rb = conf::vr::RulesBuilder{};

    rb.configureRoot(
        Title("TLS Configuration"_el), Description("Shared settings and independent client/server profiles."_el));
    rb.addRule(
        cSystemTrust,
        RuleType::Boolean,
        Title("System Trust"_el),
        Description("Override the role default for system trust."_el),
        IsOptional());
    rb.addRule(
        "trust_allow_network"_el,
        RuleType::Boolean,
        Title("Trust Network Retrieval"_el),
        Description("Allow native trust evaluation to retrieve certificate and revocation information."_el),
        IsOptional());
    rb.addRule(
        "trust_bundle_file"_el,
        RuleType::Text,
        Title("System Trust Bundle"_el),
        Description("Override the discovered Linux system certificate bundle."_el),
        Minimum(1),
        IsOptional());
    rb.addRule(
        "identity_allow_interaction"_el,
        RuleType::Boolean,
        Title("Identity Interaction"_el),
        Description("Allow native identity loading and signing to prompt the user."_el),
        IsOptional());
    for (const auto role : {"client"_el, "server"_el}) {
        rb.addRule(
            role,
            RuleType::SectionList,
            Title("Role Profiles"_el),
            Description("Independent profiles for this connection role."_el),
            Maximum(256),
            IsOptional());
        auto profiles = rb.createBranch(role);
        profiles.addRule(
            "vr_entry"_el,
            RuleType::Section,
            Title("TLS Profile"_el),
            Description("Identity and peer trust for one labeled role."_el),
            Dependency(
                conf::vr::DependencyMode::XNOR,
                {"certificate_chain_file"_el},
                {"signing_key_file"_el},
                "A certificate chain and signing key must be configured together."_el),
            Dependency(
                conf::vr::DependencyMode::If,
                {"intermediate_certificates_file"_el},
                {"trust_anchors_file"_el},
                "Intermediate certificates require explicit trust anchors."_el));
        auto profile = profiles.createBranch("vr_entry"_el);
        profile.addRule(
            "label"_el,
            RuleType::Text,
            Title("Label"_el),
            Description("Nonempty slash-delimited configuration label."_el),
            Minimum(1),
            Maximum(CryptologyConfiguration::cMaximumTlsConfigurationLabelLength),
            Matches("^[a-z0-9]+(-[a-z0-9]+)*(/[a-z0-9]+(-[a-z0-9]+)*){0,15}$"_el));
        profile.addRule(
            "certificate_chain_file"_el,
            RuleType::Text,
            Title("Certificate Chain"_el),
            Description("PEM identity chain, starting with the leaf certificate."_el),
            Minimum(1),
            IsOptional());
        profile.addRule(
            "signing_key_file"_el,
            RuleType::Text,
            Title("Signing Key"_el),
            Description("PEM private key matching the identity leaf certificate."_el),
            Minimum(1),
            IsOptional());
        profile.addRule(
            "trust_anchors_file"_el,
            RuleType::Text,
            Title("Trust Anchors"_el),
            Description("PEM trust anchors; implicitly selects explicit trust."_el),
            Minimum(1),
            IsOptional());
        profile.addRule(
            "intermediate_certificates_file"_el,
            RuleType::Text,
            Title("Intermediate Certificates"_el),
            Description("PEM untrusted issuer candidates for explicit-anchor validation."_el),
            Minimum(1),
            IsOptional());
        profile.addRule(
            "signing_key_password"_el,
            RuleType::Text,
            Title("Signing Key Password"_el),
            Description("Secret password for an encrypted signing key file."_el),
            IsSecret(),
            IsOptional());
        profile.addRule(
            "identity_fingerprint"_el,
            RuleType::Bytes,
            Title("Identity Fingerprint"_el),
            Description("SHA-256 fingerprint selecting a native identity instead of files."_el),
            Minimum(32),
            Maximum(32),
            IsOptional());
        profile.addRule(
            cSystemTrust,
            RuleType::Boolean,
            Title("System Trust"_el),
            Description("Override shared trust selection for this profile."_el),
            IsOptional());
    }
    rb.addRule(
        "server.vr_entry.client_authentication"_el,
        RuleType::Text,
        Title("Client Authentication"_el),
        Description("Disable, optionally request, or require a verified client certificate."_el),
        In({"disabled"_el, "optional"_el, "required"_el}),
        IsOptional());
    return rb.takeRules();
}

}
