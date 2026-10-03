// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../MailCommon/src/MailConfiguration.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/HostName.hpp>
#include <erbsland/IpAddress.hpp>
#include <erbsland/IpEndpoint.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/Port.hpp>
#include <erbsland/String.hpp>
#include <erbsland/TlsConfigurationMapping.hpp>

#include <vector>

namespace demo {

using namespace el::text::literals;

/// Listener address and TLS policy loaded before accepting connections.
struct ReceiverSettings {
public:
    el::IpEndpoint endpoint;                           ///< Local listening endpoint.
    el::String tlsProfile;                             ///< Default server profile label.
    el::Path logFile;                                  ///< Optional append-only log file.
    std::vector<el::TlsConfigurationMapping> mappings; ///< Exact SNI profile mappings.

public:                                                // configuration loading
    /// Validate the main section and apply explicit command-line overrides.
    /// @param configuration Parsed configuration and command-line values.
    /// @return Validated receiver settings.
    static auto load(const MailConfiguration &configuration) -> ReceiverSettings {
        using namespace el::conf::vr;
        using namespace el::conf::vr::builder;
        auto rules = RulesBuilder{};
        rules.addRule("address"_el, RuleType::Text, Title{"Listen address"_el}, Default{"127.0.0.1"_el});
        rules.addRule("port"_el, RuleType::Integer, Title{"Listen port"_el}, Minimum{0}, Maximum{65535});
        rules.addRule("tls_profile"_el, RuleType::Text, Title{"Default TLS server profile"_el}, Default{"tls"_el});
        rules.addRule("log_file"_el, RuleType::Text, Title{"Optional log file"_el}, Default{""_el});
        rules.addRule("sni"_el, RuleType::SectionList, Title{"SNI profiles"_el}, Maximum{64}, IsOptional{});
        rules.addRule("sni.vr_entry"_el, RuleType::Section, Title{"SNI mapping"_el});
        rules.addRule("sni.vr_entry.name"_el, RuleType::Text, Title{"Requested server name"_el});
        rules.addRule(
            "sni.vr_entry.profile"_el,
            RuleType::Text,
            Title{"TLS server profile"_el},
            Minimum{1},
            Maximum{255},
            Matches{"^[a-z0-9]+(-[a-z0-9]+)*(/[a-z0-9]+(-[a-z0-9]+)*){0,15}$"_el});
        rules.takeRules()->validate(configuration.main(), 1);
        auto result = ReceiverSettings{};
        result.endpoint = el::IpEndpoint{
            el::IpAddress::fromStringOrThrow(configuration.text("address"_el)),
            el::Port::fromStringOrThrow(configuration.portText("port"_el))};
        result.tlsProfile = configuration.profile();
        const auto logFile = configuration.text("log_file"_el);
        if (!logFile.isEmpty()) {
            result.logFile = configuration.resolvePath(logFile);
        }
        if (const auto mappings = configuration.main()->value("sni"_el)) {
            for (const auto &mapping : *mappings) {
                const auto name = el::HostName::fromStringOrThrow(mapping->getTextOrThrow("name"_el));
                for (const auto &existing : result.mappings) {
                    if (existing.serverName() == name) {
                        throw el::ApplicationError{"Duplicate SNI mapping."_el};
                    }
                }
                result.mappings.emplace_back(name, mapping->getTextOrThrow("profile"_el));
            }
        }
        return result;
    }
};

}
