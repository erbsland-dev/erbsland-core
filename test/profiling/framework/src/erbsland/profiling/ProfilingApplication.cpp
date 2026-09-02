// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProfilingApplication.hpp"

#include "ConfigurationLoader.hpp"
#include "WorkloadRunner.hpp"

#include <erbsland/conf/Parser.hpp>

#include <optional>

namespace erbsland::profiling {

using namespace text::literals;

void ProfilingApplication::registerProfilingOptions([[maybe_unused]] const OptionsPtr &options) {
}

void ProfilingApplication::initialize() {
    configureProfiling(_definition);
    info().setApplicationName(_definition.applicationName());
    info().setApplicationVersion(_definition.applicationVersion());
}

void ProfilingApplication::registerCommandLineOptions(const OptionsPtr &options) {
    options->setHelpTitle(_definition.helpTitle());
    options->setHelpDescription(_definition.helpDescription());
    options->addOption({"--config"_el, "config"_el})
        .setType(OptionType::Text)
        .setValueName("file"_el)
        .setHelpDescription("Load an ELCL profiling configuration."_el);
    options->addOption({"--mode"_el, "mode"_el})
        .setType(OptionType::Choice)
        .addChoice("profile"_el)
        .addChoice("benchmark"_el)
        .setValueName("mode"_el)
        .setHelpDescription("Override the configured execution mode."_el);
    options->addOption({"--threads"_el, "threads"_el})
        .setType(OptionType::Integer)
        .setValueName("count"_el)
        .setHelpDescription("Override independent workload threads (1-256)."_el);
    options->addOption({"--suite"_el, "suite"_el})
        .setType(OptionType::Text)
        .setValueName("name"_el)
        .setHelpDescription("Override the configured built-in suite."_el);
    options->addOption({"--duration"_el, "duration"_el})
        .setType(OptionType::Text)
        .setValueName("time-delta"_el)
        .setHelpDescription("Override the run duration using an ELCL time delta."_el);
    options->addOption({"--seed"_el, "seed"_el})
        .setType(OptionType::Integer)
        .setValueName("integer"_el)
        .setHelpDescription("Override the deterministic global seed."_el);
    options->addOption({"--warmup-samples"_el, "warmup-samples"_el})
        .setType(OptionType::Integer)
        .setValueName("count"_el)
        .setHelpDescription("Override the warm-up sample count."_el);
    options->addOption({"--samples"_el, "samples"_el})
        .setType(OptionType::Integer)
        .setValueName("count"_el)
        .setHelpDescription("Override the measured benchmark sample count."_el);
    options->addOption({"--minimum-sample-time"_el, "minimum-sample-time"_el})
        .setType(OptionType::Text)
        .setValueName("time-delta"_el)
        .setHelpDescription("Override the calibration target using an ELCL time delta."_el);
    options->addOption({"--memory-limit"_el, "memory-limit"_el})
        .setType(OptionType::Text)
        .setValueName("byte-count"_el)
        .setHelpDescription("Override the fixture memory limit using an ELCL byte count."_el);
    options->addOption({"--progress-interval"_el, "progress-interval"_el})
        .setType(OptionType::Text)
        .setValueName("time-delta"_el)
        .setHelpDescription("Override the progress interval using an ELCL time delta."_el);
    options->addOption({"--scenario"_el, "scenario"_el})
        .setType(OptionType::Text)
        .setMaximum(ArgumentCount::infinite())
        .setValueName("id-or-group"_el)
        .setHelpDescription("Select expanded scenario IDs or groups; repeat as needed."_el);
    options->addOption({"--list-scenarios"_el, "list-scenarios"_el})
        .setHelpDescription("Print expanded scenario identifiers and exit."_el);
    options->addOption({"--list-coverage"_el, "list-coverage"_el})
        .setHelpDescription("Print registered public API coverage and exit."_el);
    options->addOption({"--dry-run"_el, "dry-run"_el})
        .setHelpDescription("Validate and print expanded work without executing it."_el);
    options->addOption({"--write-template"_el, "write-template"_el})
        .setType(OptionType::Text)
        .setValueName("file"_el)
        .setHelpDescription("Write the complete default ELCL configuration and exit."_el);
    for (const auto &axis : _definition.axes()) {
        options->addOption({StringFormat{"--{}"_el}.build(axis.optionName()), axis.optionName()})
            .setType(OptionType::Text)
            .setMaximum(ArgumentCount::infinite())
            .setValueName(axis.id())
            .setHelpDescription(StringFormat{"Filter the '{}' profiling axis."_el}.build(axis.id()));
    }
    registerProfilingOptions(options);
}

auto ProfilingApplication::main() -> ExitCode {
    if (!optionValues()->valueCount("write-template"_el).isZero() && !_definition.defaultConfiguration().isEmpty()) {
        const auto path = Path{optionValues()->getText("write-template"_el)};
        ConfigurationLoader::writeTemplate(_definition, path);
        io::printLine("record=summary action=write-template path="_el, path.toString());
        return ExitCode::success();
    }
    return executeProfiling();
}

auto ProfilingApplication::runRegisteredProfiling() -> ExitCode {
    auto path = std::optional<Path>{};
    if (!optionValues()->valueCount("config"_el).isZero()) {
        path = Path{optionValues()->getText("config"_el)};
    }
    auto configuration = ConfigurationLoader::load(_definition, path);
    if (!optionValues()->valueCount("mode"_el).isZero()) {
        configuration.run.mode =
            optionValues()->getText("mode"_el) == "profile"_el ? RunMode::Profile : RunMode::Benchmark;
    }
    if (!optionValues()->valueCount("threads"_el).isZero()) {
        const auto threads = optionValues()->getInteger("threads"_el);
        if (threads < 1 || threads > 256) {
            throw ApplicationError{"The workload thread count must be in the range 1-256."_el};
        }
        configuration.run.threadCount = static_cast<std::uint32_t>(threads);
    }
    if (!optionValues()->valueCount("suite"_el).isZero()) {
        const auto *suite = _definition.findSuite(optionValues()->getText("suite"_el));
        if (suite == nullptr) {
            throw ApplicationError{"The requested profiling suite is not registered."_el};
        }
        configuration.run.suite = suite->id;
        configuration.scenarios = suite->scenarios;
    }
    if (!optionValues()->valueCount("duration"_el).isZero()) {
        configuration.run.duration = parseTimeDeltaOverride(optionValues()->getText("duration"_el));
    }
    if (!optionValues()->valueCount("seed"_el).isZero()) {
        const auto seed = optionValues()->getInteger("seed"_el);
        if (seed < 0) {
            throw ApplicationError{"The profiling seed must not be negative."_el};
        }
        configuration.run.seed = static_cast<std::uint64_t>(seed);
    }
    if (!optionValues()->valueCount("warmup-samples"_el).isZero()) {
        const auto samples = optionValues()->getInteger("warmup-samples"_el);
        if (samples < 0) {
            throw ApplicationError{"The warm-up sample count must not be negative."_el};
        }
        configuration.run.warmupSamples = static_cast<std::uint32_t>(samples);
    }
    if (!optionValues()->valueCount("samples"_el).isZero()) {
        const auto samples = optionValues()->getInteger("samples"_el);
        if (samples < 1) {
            throw ApplicationError{"The benchmark sample count must be positive."_el};
        }
        configuration.run.samples = static_cast<std::uint32_t>(samples);
    }
    if (!optionValues()->valueCount("minimum-sample-time"_el).isZero()) {
        configuration.run.minimumSampleTime = parseTimeDeltaOverride(optionValues()->getText("minimum-sample-time"_el));
    }
    if (!optionValues()->valueCount("memory-limit"_el).isZero()) {
        configuration.run.memoryLimit = parseByteLengthOverride(optionValues()->getText("memory-limit"_el));
    }
    if (!optionValues()->valueCount("progress-interval"_el).isZero()) {
        configuration.run.progressInterval = parseTimeDeltaOverride(optionValues()->getText("progress-interval"_el));
    }
    const auto scenarioFilters = optionValues()->getTextList("scenario"_el);
    configuration.scenarios.removeIf([&](const Scenario &scenario) -> bool {
        if (scenarioFilters.isEmpty()) {
            return false;
        }
        for (const auto &filter : scenarioFilters) {
            if (scenario.id == filter || scenario.group == filter) {
                return false;
            }
        }
        return true;
    });
    for (const auto &axis : _definition.axes()) {
        const auto filters = optionValues()->getTextList(axis.optionName());
        if (filters.isEmpty()) {
            continue;
        }
        for (const auto &filter : filters) {
            if (!axis.hasValue(filter)) {
                throw ApplicationError{
                    StringFormat{"Unsupported value '{}' for profiling axis '{}'."_el}.build(filter, axis.id())};
            }
        }
        configuration.scenarios.removeIf([&](const Scenario &scenario) -> bool {
            for (const auto &selection : scenario.axes) {
                if (selection.axis != axis.id()) {
                    continue;
                }
                for (const auto &filter : filters) {
                    if (selection.value == filter) {
                        return false;
                    }
                }
            }
            return true;
        });
    }
    if (configuration.scenarios.isEmpty() && !optionValues()->getFlag("list-coverage"_el)) {
        throw ApplicationError{"No expanded scenario matches the requested filters."_el};
    }
    ConfigurationLoader::updateDigest(configuration);
    auto runner = WorkloadRunner{_definition, std::move(configuration)};
    if (optionValues()->getFlag("list-scenarios"_el) || optionValues()->getFlag("dry-run"_el)) {
        runner.printDryRun();
        return ExitCode::success();
    }
    if (optionValues()->getFlag("list-coverage"_el)) {
        runner.printCoverage();
        return ExitCode::success();
    }
    return runner.run();
}

auto ProfilingApplication::parseTimeDeltaOverride(const String &value) -> TimeDelta {
    const auto source = StringFormat{"@version: \"1.0\"\n---[ Override ]---\nValue : {}\n"_el}.build(value);
    const auto result = conf::Parser{}
                            .parseTextOrThrow(source)
                            ->valueOrThrow("override"_el)
                            ->getCalendarDeltaOrThrow("value"_el)
                            .toTimeDeltaOrThrow();
    if (result <= TimeDelta{}) {
        throw ApplicationError{"A profiling time-delta override must be positive."_el};
    }
    return result;
}

auto ProfilingApplication::parseByteLengthOverride(const String &value) -> ByteLength {
    const auto source = StringFormat{"@version: \"1.0\"\n---[ Override ]---\nValue : {}\n"_el}.build(value);
    const auto result =
        conf::Parser{}.parseTextOrThrow(source)->valueOrThrow("override"_el)->getIntegerOrThrow("value"_el);
    if (result < 1) {
        throw ApplicationError{"A profiling byte-count override must be positive."_el};
    }
    return ByteLength{static_cast<std::uint64_t>(result)};
}

}
