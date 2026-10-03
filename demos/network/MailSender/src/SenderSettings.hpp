// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../MailCommon/src/MailConfiguration.hpp"
#include "../../MailCommon/src/MailMessage.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/Host.hpp>
#include <erbsland/HostEndpoint.hpp>
#include <erbsland/Port.hpp>
#include <erbsland/String.hpp>

namespace demo {

using namespace el::text::literals;

/// Validated destination, profile, and message for one delivery.
struct SenderSettings {
public:
    el::HostEndpoint endpoint; ///< Remote SMTP-over-TLS endpoint.
    el::String tlsProfile;     ///< Client configuration label.
    MailMessage mail;          ///< Envelope and message content.

public:                        // configuration loading
    /// Validate the main section and apply explicit command-line overrides.
    /// @param configuration The parsed application configuration.
    /// @return Ready-to-use sender settings.
    static auto load(const MailConfiguration &configuration) -> SenderSettings {
        using namespace el::conf::vr;
        using namespace el::conf::vr::builder;
        auto rules = RulesBuilder{};
        rules.addRule("smtp_host"_el, RuleType::Text, Title{"SMTP host"_el}, Minimum{1});
        rules.addRule("smtp_port"_el, RuleType::Integer, Title{"SMTP port"_el}, Minimum{1}, Maximum{65535});
        rules.addRule("tls_profile"_el, RuleType::Text, Title{"TLS client profile"_el}, Default{"tls"_el});
        for (const auto &name : {"sender_email"_el, "receiver_email"_el, "message"_el}) {
            rules.addRule(name, RuleType::Text, Title{name});
        }
        for (const auto &name : {"sender_name"_el, "receiver_name"_el}) {
            rules.addRule(name, RuleType::Text, Title{name}, Default{""_el});
        }
        rules.takeRules()->validate(configuration.main(), 1);
        const auto port = el::Port::fromStringOrThrow(configuration.portText("smtp_port"_el));
        if (port.isAutomatic()) {
            throw el::ApplicationError{"The SMTP destination port must not be zero."_el};
        }
        return {
            el::HostEndpoint{el::Host::fromStringOrThrow(configuration.text("smtp_host"_el)), port},
            configuration.profile(),
            {configuration.text("sender_email"_el),
                configuration.text("sender_name"_el),
                configuration.text("receiver_email"_el),
                configuration.text("receiver_name"_el),
                configuration.text("message"_el)}};
    }
};

}
