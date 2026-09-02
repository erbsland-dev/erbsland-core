// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Application_fwd.hpp"
#include "DefaultConfiguration.hpp"
#include "ProfileTypes.hpp"

#include "impl/BuiltInConfiguration.hpp"
#include "impl/WorkloadRunner.hpp"

#include <erbsland/conf/Parser.hpp>
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
        auto backendAxis = pf::AxisDefinition{"backend"_el, "backends"_el, "backend"_el};
        backendAxis.addValue({.id = "erbsland"_el}).addValue({.id = "std"_el});
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
        definition.addAxis(std::move(useCaseAxis)).addAxis(std::move(inputAxis)).addAxis(std::move(backendAxis));
    }

    void registerProfilingOptions(const el::OptionsPtr &options) override {
        addRepeatableFilter(options, "--pattern"_el, "pattern"_el, "name"_el, "Select pattern identifiers."_el);
        addRepeatableFilter(options, "--corpus"_el, "corpus"_el, "name"_el, "Select corpus identifiers."_el);
    }

    [[nodiscard]] auto executeProfiling() -> el::ExitCode override {
        const auto values = optionValues();
        if (!values->valueCount("write-template"_el).isZero()) {
            const auto path = el::Path{values->getText("write-template"_el)};
            Configuration::writeTemplate(path);
            el::io::printLine("template="_el, path.toString());
            return flushOutput(el::ExitCode::success());
        }
        auto configPath = std::optional<el::Path>{};
        if (!values->valueCount("config"_el).isZero()) {
            configPath = el::Path{values->getText("config"_el)};
        }
        auto configuration = Configuration::load(configPath);
        applyOverrides(configuration, values);
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
    /// Apply command-line configuration overrides.
    static void applyOverrides(Configuration &configuration, const el::OptionValuesPtr &values) {
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
        if (!values->valueCount("suite"_el).isZero()) {
            configuration.run.suite = values->getText("suite"_el);
            configuration.scenarios = impl::BuiltInConfiguration::suite(configuration.run);
        }
        if (!values->valueCount("duration"_el).isZero()) {
            configuration.run.duration = parseTimeDelta(values->getText("duration"_el));
        }
        if (!values->valueCount("seed"_el).isZero()) {
            const auto seed = values->getInteger("seed"_el);
            if (seed < 0) {
                throw el::ApplicationError{"The profiling seed must not be negative."_el};
            }
            configuration.run.seed = static_cast<std::uint64_t>(seed);
        }
        if (!values->valueCount("warmup-samples"_el).isZero()) {
            const auto samples = values->getInteger("warmup-samples"_el);
            if (samples < 0) {
                throw el::ApplicationError{"The warm-up sample count must not be negative."_el};
            }
            configuration.run.warmupSamples = static_cast<std::uint32_t>(samples);
        }
        if (!values->valueCount("samples"_el).isZero()) {
            const auto samples = values->getInteger("samples"_el);
            if (samples < 1) {
                throw el::ApplicationError{"The benchmark sample count must be positive."_el};
            }
            configuration.run.samples = static_cast<std::uint32_t>(samples);
        }
        if (!values->valueCount("minimum-sample-time"_el).isZero()) {
            configuration.run.minimumSampleTime = parseTimeDelta(values->getText("minimum-sample-time"_el));
        }
        if (!values->valueCount("memory-limit"_el).isZero()) {
            configuration.run.memoryLimit = parseByteCount(values->getText("memory-limit"_el));
        }
        if (!values->valueCount("progress-interval"_el).isZero()) {
            configuration.run.progressInterval = parseTimeDelta(values->getText("progress-interval"_el));
        }
    }

    /// Parse one configuration override value.
    [[nodiscard]] static auto parseOverride(const el::String &value) -> el::conf::ValuePtr {
        const auto source = el::StringFormat{"@version: \"1.0\"\n---[ Override ]---\nValue : {}\n"_el}.build(value);
        return el::conf::Parser{}.parseTextOrThrow(source)->valueOrThrow("override"_el)->valueOrThrow("value"_el);
    }

    /// Parse a positive duration override.
    [[nodiscard]] static auto parseTimeDelta(const el::String &value) -> std::chrono::nanoseconds {
        const auto result = parseOverride(value)->asCalendarDeltaOrThrow().toTimeDeltaOrThrow().toStdNanoseconds();
        if (result <= std::chrono::nanoseconds::zero()) {
            throw el::ApplicationError{"A profiling time-delta override must be positive."_el};
        }
        return result;
    }

    /// Parse a positive byte-count override.
    [[nodiscard]] static auto parseByteCount(const el::String &value) -> std::uint64_t {
        const auto result = parseOverride(value)->asIntegerOrThrow();
        if (result < 1) {
            throw el::ApplicationError{"A profiling byte-count override must be positive."_el};
        }
        return static_cast<std::uint64_t>(result);
    }

    /// Add one repeatable command-line filter option.
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

    /// Filter scenarios according to command-line selections.
    static void filter(Configuration &configuration, const el::OptionValuesPtr &values) {
        const auto useCases = values->getTextList("use-case"_el);
        const auto inputs = values->getTextList("input"_el);
        const auto patterns = values->getTextList("pattern"_el);
        const auto corpora = values->getTextList("corpus"_el);
        const auto scenarios = values->getTextList("scenario"_el);
        const auto backends = values->getTextList("backend"_el);
        std::erase_if(configuration.scenarios, [&](const Scenario &scenario) {
            const auto useCaseMatches = useCases.isEmpty() || contains(useCases, toString(scenario.useCase));
            const auto inputMatches = inputs.isEmpty() || contains(inputs, toString(scenario.inputKind));
            const auto patternMatches = patterns.isEmpty() || contains(patterns, scenario.patternName);
            const auto corpusMatches = corpora.isEmpty() || contains(corpora, scenario.corpusName);
            const auto scenarioMatches =
                scenarios.isEmpty() || contains(scenarios, scenario.id) || contains(scenarios, scenario.group);
            const auto backendMatches = backends.isEmpty() || contains(backends, toString(scenario.backend));
            return !useCaseMatches || !inputMatches || !patternMatches || !corpusMatches || !scenarioMatches ||
                !backendMatches;
        });
    }

    /// Test if a string list contains a value.
    [[nodiscard]] static auto contains(const el::StringList &values, const el::String &value) -> bool {
        return std::ranges::find(values, value) != values.end();
    }

    /// Flush standard output before returning an exit code.
    [[nodiscard]] static auto flushOutput(const el::ExitCode exitCode) -> el::ExitCode {
        el::stdOut()->flush();
        return exitCode;
    }
};

}
