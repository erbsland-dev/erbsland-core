// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>

using namespace el::conf;
using namespace el::text::literals;
using namespace el::conf::vr::builder;
using vr::RulesBuilder;
using vr::RuleType;

// Testing manual rule construction.
TESTED_TARGETS(RulesBuilder RuleDefinition)
TAGS(ValidationRules)
class VrManualConstructionTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void testBasicConstruction() {
        RulesBuilder rulesBuilder;
        rulesBuilder.addRule(el::text::String{"app"}, RuleType::Section);
        rulesBuilder.addRule(
            el::text::String{"app.x"},
            RuleType::Integer,
            Title(el::text::String{"X"}),
            Description(el::text::String{"This is the value x"}),
            Minimum(1),
            Maximum(100));
        rules = rulesBuilder.takeRules();
        REQUIRE(rules);
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "x = 10",
        }));
        auto xValue = document->value(el::text::String{"app.x"});
        REQUIRE(xValue);
        const auto validationRule = xValue->validationRule();
        REQUIRE(validationRule);
        REQUIRE_EQUAL(validationRule->type(), RuleType::Integer);
        REQUIRE_EQUAL(validationRule->title(), el::text::String{"X"});
        REQUIRE_EQUAL(validationRule->description(), el::text::String{"This is the value x"});
    }

    void testAdvancedConstruction() {
        RulesBuilder rulesBuilder;
        rulesBuilder.addRule(
            el::text::String{"app"},
            RuleType::Section,
            KeyIndex(el::text::String{"user_id"_el}, NamePathLike{el::text::String{"users.vr_entry.id"_el}}));
        rulesBuilder.addRule(
            el::text::String{"app.server"},
            RuleType::Section,
            IsOptional(),
            Dependency(
                el::conf::vr::DependencyMode::XOR,
                {NamePathLike{el::text::String{"hostname"}}},
                {NamePathLike{el::text::String{"ip_address"}}}));
        rulesBuilder.addRule(el::text::String{"app.server.hostname"}, RuleType::Text, IsOptional());
        rulesBuilder.addRule(el::text::String{"app.server.ip_address"}, RuleType::Text, IsOptional());

        rulesBuilder.addRule(el::text::String{"app.users"}, RuleType::SectionList);
        rulesBuilder.addRule(el::text::String{"app.users.vr_entry"}, RuleType::Section);
        rulesBuilder.addRule(el::text::String{"app.users.vr_entry.id"}, RuleType::Integer);
        rulesBuilder.addRule(
            el::text::String{"app.users.vr_entry.name"}, RuleType::Text, Starts(el::text::String{"u"}));

        rulesBuilder.addRule(
            el::text::String{"app.start_user_id"},
            RuleType::Integer,
            ConfKey(NamePathLike{el::text::String{"user_id"}}));

        rulesBuilder.addRule(
            el::text::String{"app.mode"},
            RuleType::Text,
            Default(el::text::String{"dev"}),
            In({el::text::String{"dev"}, el::text::String{"prod"}}),
            ConfVersion({1, 2, 3}),
            MinimumVersion(1),
            MaximumVersion(10));

        rules = rulesBuilder.takeRules();
        REQUIRE(rules);

        WITH_CONTEXT(requirePassLines(
            {
                "[app]",
                "start_user_id: 1",
                "mode: \"prod\"",
                "[app.server]",
                "hostname: \"example.local\"",
                "*[app.users]*",
                "id: 1",
                "name: \"user-1\"",
            },
            2));

        WITH_CONTEXT(requireFailLines(
            {
                "[app]",
                "start_user_id: 99",
                "mode: \"prod\"",
                "*[app.users]*",
                "id: 1",
                "name: \"user-1\"",
            },
            2));
        WITH_CONTEXT(requireError("must refer to an existing key"));

        WITH_CONTEXT(requireFailLines(
            {
                "[app]",
                "start_user_id: 1",
                "[app.server]",
                "hostname: \"example.local\"",
                "ip_address: \"127.0.0.1\"",
                "*[app.users]*",
                "id: 1",
                "name: \"user-1\"",
            },
            2));
        WITH_CONTEXT(requireError("either configure 'hostname' or configure 'ip_address'"));
    }
};
