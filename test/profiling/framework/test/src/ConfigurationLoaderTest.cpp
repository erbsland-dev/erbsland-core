// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/profiling/ConfigurationLoader.hpp>
#include <erbsland/unittest/UnitTest.hpp>

namespace pf = erbsland::profiling;

using namespace el::text::literals;

TESTED_TARGETS(ConfigurationLoader ProfilingConfiguration RunConfiguration Scenario)
class ConfigurationLoaderTest final : public el::UnitTest {
public:
    void testSuiteLoading() {
        auto definition = pf::ProfilingDefinition{};
        definition.setDefaultConfiguration(
            "@version: \"1.0\"\n---[ Run ]---\nMode : \"benchmark\"\nSuite : \"smoke\"\nThreads : 1\n"_el);
        definition.addFunctionality({.id = "noop"_el});
        definition.addSuite(
            {.id = "smoke"_el,
                .scenarios = el::List<pf::Scenario>{
                    pf::Scenario{.id = "smoke:noop"_el, .group = "smoke"_el, .functionality = "noop"_el}}});
        const auto configuration = pf::ConfigurationLoader::load(definition);
        REQUIRE_EQUAL(configuration.run.threadCount, 1U);
        REQUIRE_EQUAL(configuration.scenarios.count().toRawValue(), 1);
        REQUIRE_EQUAL(configuration.scenarios.first().id, "smoke:noop"_el);
    }

    void testUnknownRunValue() {
        auto definition = pf::ProfilingDefinition{};
        definition.setDefaultConfiguration("@version: \"1.0\"\n---[ Run ]---\nMode : \"benchmark\"\nUnknown : 1\n"_el);
        REQUIRE_THROWS_AS(el::ApplicationError, pf::ConfigurationLoader::load(definition));
    }

    void testCartesianAxisExpansionAndTypedParameter() {
        auto definition = pf::ProfilingDefinition{};
        definition.setDefaultConfiguration(
            "@version: \"1.0\"\n"
            "---[ Run ]---\nMode : \"benchmark\"\n"
            "--*[ Scenario ]*---\n"
            "Name : \"matrix\"\nFunctionality : \"noop\"\nWidth : \"u8\", \"u16\"\nInput Size : 4 KiB\n"_el);
        auto axis = pf::AxisDefinition{"width"_el, "width"_el, "width"_el};
        axis.addValue({.id = "u8"_el}).addValue({.id = "u16"_el});
        definition.addAxis(std::move(axis))
            .addParameter(
                {.id = "input-size"_el, .configurationName = "input_size"_el, .type = pf::ParameterType::ByteLength})
            .addFunctionality({.id = "noop"_el});
        const auto configuration = pf::ConfigurationLoader::load(definition);
        REQUIRE_EQUAL(configuration.scenarios.count().toRawValue(), 2);
        REQUIRE_EQUAL(configuration.scenarios.first().id, "matrix:noop:u8"_el);
        REQUIRE_EQUAL(
            std::get<el::ByteLength>(configuration.scenarios.first().parameters.first().value), el::ByteLength{4096U});
    }

    void testCompatibilityRejection() {
        auto definition = pf::ProfilingDefinition{};
        definition.setDefaultConfiguration(
            "@version: \"1.0\"\n---[ Run ]---\nMode : \"benchmark\"\n"
            "--*[ Scenario ]*---\nName : \"bad\"\nFunctionality : \"noop\"\nWidth : \"u32\"\n"_el);
        auto axis = pf::AxisDefinition{"width"_el, "width"_el, "width"_el};
        axis.addValue({.id = "u32"_el});
        definition.addAxis(std::move(axis))
            .addFunctionality({.id = "noop"_el})
            .setCompatibility([](const pf::Scenario &) -> bool { return false; });
        REQUIRE_THROWS_AS(el::ApplicationError, pf::ConfigurationLoader::load(definition));
    }

    void testDuplicateExpandedIdentifier() {
        auto definition = pf::ProfilingDefinition{};
        definition.setDefaultConfiguration(
            "@version: \"1.0\"\n---[ Run ]---\nMode : \"benchmark\"\n"
            "--*[ Scenario ]*---\nName : \"same\"\nFunctionality : \"noop\"\n"
            "--*[ Scenario ]*---\nName : \"same\"\nFunctionality : \"noop\"\n"_el);
        definition.addFunctionality({.id = "noop"_el});
        REQUIRE_THROWS_AS(el::ApplicationError, pf::ConfigurationLoader::load(definition));
    }
};
