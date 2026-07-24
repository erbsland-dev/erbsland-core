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

namespace app::string {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

/// The string-types profiler command-line application.
/// @notest{Covered by registered string profiler CTest entries.}
class StringTypesProfileApplication final : public pf::ProfilingApplication {
public:
    using ProfilingApplication::ProfilingApplication;

protected: // implement pf::ProfilingApplication
    void configureProfiling(pf::ProfilingDefinition &definition) override {
        definition
            .setApplication(
                "String Types Profiling"_el,
                el::Version{1, 0, 0},
                "String Types Profiler and Benchmark"_el,
                "Profiles deterministic U8, U16, and U32 String and StringEditor workloads. With no options, it "
                "runs the four-thread snapshot benchmark."_el)
            .setDefaultConfiguration(el::String{cDefaultConfigurationText});
        auto widthAxis = pf::AxisDefinition{"width"_el, "widths"_el, "width"_el};
        for (const auto width : {StringWidth::U8, StringWidth::U16, StringWidth::U32}) {
            widthAxis.addValue({.id = toString(width)});
        }
        auto typeAxis = pf::AxisDefinition{"type"_el, "types"_el, "type"_el};
        for (const auto type : {StringType::String, StringType::StringEditor}) {
            typeAxis.addValue({.id = toString(type)});
        }
        auto useCaseAxis = pf::AxisDefinition{"use-case"_el, "use_cases"_el, "use-case"_el};
        for (const auto &descriptor : coverageRegistry()) {
            const auto id = toString(descriptor.useCase);
            if (!useCaseAxis.hasValue(id)) {
                useCaseAxis.addValue({.id = id});
            }
        }
        auto contentAxis = pf::AxisDefinition{"content-profile"_el, "content_profiles"_el, "content-profile"_el};
        for (
            const auto content :
            {ContentProfile::Ascii,
                ContentProfile::Mixed,
                ContentProfile::Supplementary,
                ContentProfile::MalformedSparse,
                ContentProfile::MalformedDense}) {
            contentAxis.addValue({.id = toString(content)});
        }
        definition.addAxis(std::move(widthAxis))
            .addAxis(std::move(typeAxis))
            .addAxis(std::move(useCaseAxis))
            .addAxis(std::move(contentAxis));
    }

    void registerProfilingOptions(const el::OptionsPtr &options) override {
        options->addOption({"--sensitive-mode"_el, "sensitive-mode"_el})
            .setType(el::OptionType::Choice)
            .addChoice("normal"_el)
            .addChoice("sensitive"_el)
            .addChoice("all"_el)
            .setValueName("mode"_el)
            .setHelpDescription("Select normal, sensitive, or paired U8 storage scenarios."_el);
    }

    [[nodiscard]] auto executeProfiling() -> el::ExitCode override {
        const auto values = optionValues();
        if (!values->valueCount("write-template"_el).isZero()) {
            const auto path = el::Path{values->getText("write-template"_el)};
            ConfigurationLoader::writeTemplate(path);
            el::io::printLine("template="_el, path.toString());
            return el::ExitCode::success();
        }
        auto configPath = std::optional<el::Path>{};
        if (!values->valueCount("config"_el).isZero()) {
            configPath = el::Path{values->getText("config"_el)};
        }
        auto overrides = RunOverrides{};
        if (!values->valueCount("mode"_el).isZero()) {
            overrides.mode = values->getText("mode"_el) == "profile"_el ? RunMode::Profile : RunMode::Benchmark;
        }
        if (!values->valueCount("threads"_el).isZero()) {
            const auto threads = values->getInteger("threads"_el);
            if (threads < 1 || threads > 256) {
                throw el::ApplicationError{"The workload thread count must be in the range 1-256."_el};
            }
            overrides.threadCount = static_cast<std::uint32_t>(threads);
        }
        if (!values->valueCount("sensitive-mode"_el).isZero()) {
            overrides.sensitiveSelection = parseSensitiveSelection(values->getText("sensitive-mode"_el));
        }
        auto configuration = ConfigurationLoader::load(configPath, overrides);
        const auto widthFilters = values->getTextList("width"_el);
        const auto typeFilters = values->getTextList("type"_el);
        const auto useCaseFilters = values->getTextList("use-case"_el);
        const auto contentFilters = values->getTextList("content-profile"_el);
        const auto scenarioFilters = values->getTextList("scenario"_el);
        std::erase_if(configuration.scenarios, [&](const Scenario &scenario) {
            const auto matches = []<typename Parser, typename T>(
                                     const std::vector<el::String> &filters,
                                     Parser parser,
                                     const T expected,
                                     const el::StringLiteral kind) {
                return filters.empty() || std::ranges::any_of(filters, [&](const auto &value) {
                    const auto parsed = parser(value);
                    if (!parsed) {
                        throw el::ApplicationError{el::StringFormat{"Unsupported {} '{}'."_el}.build(kind, value)};
                    }
                    return *parsed == expected;
                });
            };
            const auto scenarioMatches =
                scenarioFilters.empty() || std::ranges::any_of(scenarioFilters, [&](const auto &value) {
                    return value == scenario.id || value == scenario.group;
                });
            return !matches(widthFilters, parseStringWidth, scenario.width, "string width"_el) ||
                !matches(typeFilters, parseStringType, scenario.type, "string type"_el) ||
                !matches(useCaseFilters, parseUseCase, scenario.useCase, "use case"_el) ||
                !matches(contentFilters, parseContentProfile, scenario.contentProfile, "content profile"_el) ||
                !scenarioMatches;
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
            static_cast<void>(el::stdOut()->flush());
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
