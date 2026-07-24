// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Application_fwd.hpp"
#include "DefaultConfiguration.hpp"

#include "impl/ConfigurationLoader.hpp"
#include "impl/WorkloadRunner.hpp"

#include <erbsland/profiling/AxisDefinition.hpp>
#include <erbsland/profiling/ProfilingApplication.hpp>

#include <algorithm>
#include <optional>

namespace app::regex {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

/// The regular-expression API profiler command-line application.
/// @notest{Covered by registered regex profiler CTest entries.}
class RegexApiProfileApplication final : public pf::ProfilingApplication {
public:
    using ProfilingApplication::ProfilingApplication;

protected: // implement pf::ProfilingApplication
    void configureProfiling(pf::ProfilingDefinition &definition) override {
        definition
            .setApplication(
                "Regular Expression API Profiling"_el,
                el::Version{1, 0, 0},
                "Regular Expression API Profiler and Benchmark"_el,
                "Profiles deterministic regular-expression compilation, matching, search, collection, "
                "replacement, string, and seekable-file workloads. With no options it runs the four-thread "
                "snapshot benchmark."_el)
            .setDefaultConfiguration(el::String{cDefaultConfigurationText});
        auto useCaseAxis = pf::AxisDefinition{"use-case"_el, "use_cases"_el, "use-case"_el};
        auto inputAxis = pf::AxisDefinition{"input"_el, "inputs"_el, "input"_el};
        for (const auto &descriptor : coverageRegistry()) {
            const auto useCase = toString(descriptor.useCase);
            if (!useCaseAxis.hasValue(useCase)) {
                useCaseAxis.addValue({.id = useCase});
            }
            const auto input = toString(descriptor.inputKind);
            if (!inputAxis.hasValue(input)) {
                inputAxis.addValue({.id = input});
            }
        }
        definition.addAxis(std::move(useCaseAxis)).addAxis(std::move(inputAxis));
    }

    void registerProfilingOptions(const el::OptionsPtr &options) override {
        addRepeatableFilter(options, "--pattern"_el, "pattern"_el, "name"_el, "Select pattern identifiers."_el);
        addRepeatableFilter(options, "--corpus"_el, "corpus"_el, "name"_el, "Select corpus identifiers."_el);
    }

    [[nodiscard]] auto executeProfiling() -> el::ExitCode override {
        const auto values = optionValues();
        if (!values->valueCount("write-template"_el).isZero()) {
            const auto path = el::Path{values->getText("write-template"_el)};
            impl::ConfigurationLoader::writeTemplate(path);
            el::io::printLine("template="_el, path.toString());
            return flushOutput(el::ExitCode::success());
        }
        auto configPath = std::optional<el::Path>{};
        if (!values->valueCount("config"_el).isZero()) {
            configPath = el::Path{values->getText("config"_el)};
        }
        auto configuration = impl::ConfigurationLoader::load(configPath);
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
        filter(configuration, values);
        if (configuration.scenarios.empty() && !values->getFlag("list-coverage"_el)) {
            throw el::ApplicationError{"No expanded scenario matches the requested filters."_el};
        }
        if (values->getFlag("list-scenarios"_el)) {
            for (const auto &scenario : configuration.scenarios) {
                el::io::printLine("record=scenario id="_el, scenario.id);
            }
            return flushOutput(el::ExitCode::success());
        }
        if (values->getFlag("list-coverage"_el)) {
            impl::WorkloadRunner::printCoverage();
            return flushOutput(el::ExitCode::success());
        }
        if (values->getFlag("dry-run"_el)) {
            impl::WorkloadRunner::printDryRun(configuration);
            return flushOutput(el::ExitCode::success());
        }
        return flushOutput(impl::WorkloadRunner::run(configuration));
    }

private:
    static void addRepeatableFilter(
        const el::OptionsPtr &options,
        const el::String &longName,
        const el::String &name,
        const el::String &valueName,
        const el::String &description) {
        options->addOption({longName, name})
            .setType(el::OptionType::Text)
            .setMaximum(el::ArgumentCount::infinite())
            .setValueName(valueName)
            .setHelpDescription(description);
    }

    static void filter(Configuration &configuration, const el::OptionValuesPtr &values) {
        const auto useCases = values->getTextList("use-case"_el);
        const auto inputs = values->getTextList("input"_el);
        const auto patterns = values->getTextList("pattern"_el);
        const auto corpora = values->getTextList("corpus"_el);
        const auto scenarios = values->getTextList("scenario"_el);
        std::erase_if(configuration.scenarios, [&](const Scenario &scenario) {
            const auto useCaseMatches = useCases.empty() || contains(useCases, toString(scenario.useCase));
            const auto inputMatches = inputs.empty() || contains(inputs, toString(scenario.inputKind));
            const auto patternMatches = patterns.empty() || contains(patterns, scenario.patternName);
            const auto corpusMatches = corpora.empty() || contains(corpora, scenario.corpusName);
            const auto scenarioMatches =
                scenarios.empty() || contains(scenarios, scenario.id) || contains(scenarios, scenario.group);
            return !useCaseMatches || !inputMatches || !patternMatches || !corpusMatches || !scenarioMatches;
        });
    }

    [[nodiscard]] static auto contains(const std::vector<el::String> &values, const el::String &value) -> bool {
        return std::ranges::find(values, value) != values.end();
    }

    [[nodiscard]] static auto flushOutput(const el::ExitCode exitCode) -> el::ExitCode {
        static_cast<void>(el::stdOut()->flush());
        return exitCode;
    }
};

}
