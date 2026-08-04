// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Application_fwd.hpp"
#include "Configuration.hpp"
#include "DefaultConfiguration.hpp"
#include "Workload.hpp"

#include <erbsland/profiling/AxisDefinition.hpp>
#include <erbsland/profiling/ProfilingApplication.hpp>

#include <algorithm>
#include <optional>

namespace app::byte {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

/// The byte-types profiler command-line application.
/// @notest{Covered by registered byte profiler CTest entries.}
class ByteTypesProfileApplication final : public pf::ProfilingApplication {
public:
    using ProfilingApplication::ProfilingApplication;

protected: // implement pf::ProfilingApplication
    void configureProfiling(pf::ProfilingDefinition &definition) override {
        definition
            .setApplication(
                "Byte Types Profiling"_el,
                el::Version{1, 0, 0},
                "Byte Types Profiler and Benchmark"_el,
                "Profiles deterministic ByteArray, ByteBlock, ByteBlockEditor, ByteBuffer, and ByteRingBuffer "
                "workloads. With no options, it runs the four-thread snapshot benchmark."_el)
            .setDefaultConfiguration(el::String{cDefaultConfigurationText});
        auto typeAxis = pf::AxisDefinition{"type"_el, "types"_el, "type"_el};
        for (
            const auto type :
            {ByteType::Array, ByteType::Block, ByteType::BlockEditor, ByteType::Buffer, ByteType::RingBuffer}) {
            typeAxis.addValue({.id = toString(type)});
        }
        auto useCaseAxis = pf::AxisDefinition{"use-case"_el, "use_cases"_el, "use-case"_el};
        for (const auto &descriptor : coverageRegistry()) {
            const auto id = toString(descriptor.useCase);
            if (!useCaseAxis.hasValue(id)) {
                useCaseAxis.addValue({.id = id});
            }
        }
        definition.addAxis(std::move(typeAxis)).addAxis(std::move(useCaseAxis));
    }

    void registerProfilingOptions(const el::OptionsPtr &options) override {
        options->addOption({"--sensitive-mode"_el, "sensitive-mode"_el})
            .setType(el::OptionType::Choice)
            .addChoice("normal"_el)
            .addChoice("sensitive"_el)
            .addChoice("all"_el)
            .setValueName("mode"_el)
            .setHelpDescription("Select normal, sensitive, or paired storage scenarios."_el);
    }

    [[nodiscard]] auto executeProfiling() -> el::ExitCode override {
        const auto values = optionValues();
        if (!values->valueCount("write-template"_el).isZero()) {
            const auto path = el::Path{values->getText("write-template"_el)};
            Configuration::writeTemplate(path);
            el::io::printLine("template="_el, path.toString());
            return el::ExitCode::success();
        }
        auto configPath = std::optional<el::Path>{};
        if (!values->valueCount("config"_el).isZero()) {
            configPath = el::Path{values->getText("config"_el)};
        }
        auto configuration = Configuration::load(configPath);
        if (!values->valueCount("mode"_el).isZero()) {
            configuration.run.mode = values->getText("mode"_el) == "profile"_el ? RunMode::Profile : RunMode::Benchmark;
        }
        if (!values->valueCount("threads"_el).isZero()) {
            const auto threads = values->getInteger("threads"_el);
            if (threads < 1 || threads > 256) {
                throw el::ApplicationError{"The workload thread count must be in the range 1-256."_el};
            }
            configuration.run.threadCount = static_cast<std::uint32_t>(threads);
        }
        if (!values->valueCount("sensitive-mode"_el).isZero()) {
            const auto selection = parseSensitiveSelection(values->getText("sensitive-mode"_el));
            if (!selection) {
                throw el::ApplicationError{"Unsupported sensitive-mode override."_el};
            }
            configuration.run.sensitiveSelection = *selection;
            if (*selection != SensitiveSelection::All) {
                const auto selected =
                    *selection == SensitiveSelection::Sensitive ? SensitiveMode::Sensitive : SensitiveMode::Normal;
                std::erase_if(configuration.scenarios, [&](const Scenario &scenario) {
                    return scenario.sensitiveMode != SensitiveMode::NotApplicable && scenario.sensitiveMode != selected;
                });
            }
        }
        const auto typeFilters = values->getTextList("type"_el);
        const auto useCaseFilters = values->getTextList("use-case"_el);
        const auto scenarioFilters = values->getTextList("scenario"_el);
        std::erase_if(configuration.scenarios, [&](const Scenario &scenario) {
            const auto typeMatches = typeFilters.empty() || std::ranges::any_of(typeFilters, [&](const auto &value) {
                const auto type = parseByteType(value);
                if (!type) {
                    throw el::ApplicationError{el::StringFormat{"Unsupported byte type '{}'."_el}.build(value)};
                }
                return *type == scenario.type;
            });
            const auto useCaseMatches =
                useCaseFilters.empty() || std::ranges::any_of(useCaseFilters, [&](const auto &value) {
                    const auto useCase = parseUseCase(value);
                    if (!useCase) {
                        throw el::ApplicationError{el::StringFormat{"Unsupported use case '{}'."_el}.build(value)};
                    }
                    return *useCase == scenario.useCase;
                });
            const auto scenarioMatches =
                scenarioFilters.empty() || std::ranges::any_of(scenarioFilters, [&](const auto &value) {
                    return value == scenario.id || value == scenario.group;
                });
            return !typeMatches || !useCaseMatches || !scenarioMatches;
        });
        if (configuration.scenarios.empty() && !values->getFlag("list-coverage"_el)) {
            throw el::ApplicationError{"No expanded scenario matches the requested filters."_el};
        }
        if (values->getFlag("list-scenarios"_el)) {
            for (const auto &scenario : configuration.scenarios) {
                el::io::printLine("record=scenario id="_el, scenario.id);
            }
            return el::ExitCode::success();
        }
        auto runner = WorkloadRunner{std::move(configuration)};
        if (values->getFlag("list-coverage"_el)) {
            runner.printCoverage();
            return el::ExitCode::success();
        }
        if (values->getFlag("dry-run"_el)) {
            runner.printDryRun();
            return el::ExitCode::success();
        }
        return runner.run();
    }
};

}
