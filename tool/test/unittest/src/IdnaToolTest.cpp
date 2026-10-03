// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/frame/ToolApplication.hpp>
#include <tool/idna/IdnaTool.hpp>

using namespace el::text::literals;

TESTED_TARGETS(IdnaTool)
class IdnaToolTest final : public el::UnitTest {
public:
    void testConfigurationValidation() {
        auto tool = tool::idna::IdnaTool{};
        REQUIRE_EQUAL(tool.identifier(), "generate_idna_data"_el);
        const auto rules = tool.configurationRules();
        REQUIRE(rules);
        auto parser = el::conf::Parser{};
        rules->validate(parser.parseTextOrThrow("[generate_idna_data]\n"_el)->value(tool.identifier()), 1);
        rules->validate(
            parser.parseTextOrThrow("[generate_idna_data]\noutput_file: \"generated.cpp\"\n"_el)
                ->value(tool.identifier()),
            1);
        REQUIRE_THROWS(rules->validate(
            parser.parseTextOrThrow("[generate_idna_data]\nunknown: 1\n"_el)->value(tool.identifier()), 1));
        REQUIRE_THROWS(rules->validate(
            parser.parseTextOrThrow("[generate_idna_data]\noutput_file: 1\n"_el)->value(tool.identifier()), 1));
    }
};
