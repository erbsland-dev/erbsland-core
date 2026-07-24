// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfigurationLoader.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/Hasher.hpp>

#include <optional>
#include <set>
#include <type_traits>

namespace erbsland::profiling {

using namespace text::literals;

void ConfigurationLoader::configurationError(const String &message) {
    throw ApplicationError{message};
}

void ConfigurationLoader::requireKnown(const conf::ValuePtr &value, const StringList &known) {
    for (const auto &entry : *value) {
        const auto name = entry->name().asText();
        if (!known.contains(name)) {
            configurationError(StringFormat{"Unknown profiling configuration value '{}'."_el}.build(name));
        }
    }
}

void ConfigurationLoader::parseRun(const conf::ValuePtr &value, RunConfiguration &run) {
    auto known = StringList{
        "mode"_el,
        "suite"_el,
        "duration"_el,
        "threads"_el,
        "seed"_el,
        "warmup_samples"_el,
        "samples"_el,
        "minimum_sample_time"_el,
        "memory_limit"_el,
        "progress_interval"_el};
    requireKnown(value, known);
    if (value->hasValue("mode"_el)) {
        const auto mode = value->getTextOrThrow("mode"_el);
        if (mode == "profile"_el) {
            run.mode = RunMode::Profile;
        } else if (mode == "benchmark"_el) {
            run.mode = RunMode::Benchmark;
        } else {
            configurationError(StringFormat{"Unsupported profiling mode '{}'."_el}.build(mode));
        }
    }
    if (value->hasValue("suite"_el)) {
        run.suite = value->getTextOrThrow("suite"_el);
    }
    if (value->hasValue("duration"_el)) {
        run.duration = value->getCalendarDeltaOrThrow("duration"_el).toTimeDeltaOrThrow();
        if (run.duration <= TimeDelta{}) {
            configurationError("The profiling duration must be positive."_el);
        }
    }
    if (value->hasValue("threads"_el)) {
        const auto threads = value->getIntegerOrThrow("threads"_el);
        if (threads < 1 || threads > 256) {
            configurationError("The profiling thread count must be in the range 1-256."_el);
        }
        run.threadCount = static_cast<std::uint32_t>(threads);
    }
    if (value->hasValue("seed"_el)) {
        const auto seed = value->getIntegerOrThrow("seed"_el);
        if (seed < 0) {
            configurationError("The profiling seed must not be negative."_el);
        }
        run.seed = static_cast<std::uint64_t>(seed);
    }
    if (value->hasValue("warmup_samples"_el)) {
        const auto samples = value->getIntegerOrThrow("warmup_samples"_el);
        if (samples < 0) {
            configurationError("The warm-up sample count must not be negative."_el);
        }
        run.warmupSamples = static_cast<std::uint32_t>(samples);
    }
    if (value->hasValue("samples"_el)) {
        const auto samples = value->getIntegerOrThrow("samples"_el);
        if (samples < 1) {
            configurationError("The benchmark sample count must be positive."_el);
        }
        run.samples = static_cast<std::uint32_t>(samples);
    }
    if (value->hasValue("minimum_sample_time"_el)) {
        run.minimumSampleTime = value->getCalendarDeltaOrThrow("minimum_sample_time"_el).toTimeDeltaOrThrow();
        if (run.minimumSampleTime <= TimeDelta{}) {
            configurationError("The minimum sample time must be positive."_el);
        }
    }
    if (value->hasValue("memory_limit"_el)) {
        const auto memoryLimit = value->getIntegerOrThrow("memory_limit"_el);
        if (memoryLimit < 1) {
            configurationError("The profiling memory limit must be positive."_el);
        }
        run.memoryLimit = ByteLength{static_cast<std::uint64_t>(memoryLimit)};
    }
    if (value->hasValue("progress_interval"_el)) {
        run.progressInterval = value->getCalendarDeltaOrThrow("progress_interval"_el).toTimeDeltaOrThrow();
        if (run.progressInterval <= TimeDelta{}) {
            configurationError("The progress interval must be positive."_el);
        }
    }
}

auto ConfigurationLoader::parseDocument(
    const ProfilingDefinition &definition, const conf::ValuePtr &document, RunConfiguration &run)
    -> std::optional<List<Scenario>> {
    requireKnown(document, StringList{"run"_el, "scenario"_el});
    if (document->hasValue("run"_el)) {
        parseRun(document->valueOrThrow("run"_el), run);
    }
    if (!document->hasValue("scenario"_el)) {
        return {};
    }
    auto scenarios = List<Scenario>{};
    for (const auto &entry : *document->valueOrThrow("scenario"_el)) {
        auto known = StringList{"name"_el, "functionality"_el, "weight"_el};
        for (const auto &axis : definition.axes()) {
            known.append(axis.configurationName());
        }
        for (const auto &parameter : definition.parameters()) {
            known.append(parameter.configurationName);
        }
        requireKnown(entry, known);
        auto base = Scenario{};
        base.group = entry->hasValue("name"_el) ? entry->getTextOrThrow("name"_el) : "custom"_el;
        base.functionality = entry->getTextOrThrow("functionality"_el);
        base.weight =
            entry->hasValue("weight"_el) ? static_cast<std::uint32_t>(entry->getIntegerOrThrow("weight"_el)) : 1U;
        if (base.weight == 0U) {
            configurationError("A profiling scenario weight must be positive."_el);
        }
        if (definition.findFunctionality(base.functionality) == nullptr) {
            configurationError(StringFormat{"Unknown profiling functionality '{}'."_el}.build(base.functionality));
        }
        auto expanded = List<Scenario>{base};
        for (const auto &axis : definition.axes()) {
            const auto name = axis.configurationName();
            if (!entry->hasValue(name)) {
                configurationError(StringFormat{"Scenario '{}' requires axis '{}'."_el}.build(base.group, name));
            }
            const auto values = StringList{entry->getListOrThrow<String>(name)};
            if (values.isEmpty()) {
                configurationError(StringFormat{"Scenario '{}' has an empty axis '{}'."_el}.build(base.group, name));
            }
            auto next = List<Scenario>{};
            for (const auto &scenario : expanded) {
                for (const auto &selected : values) {
                    if (!axis.hasValue(selected)) {
                        configurationError(
                            StringFormat{"Unknown value '{}' for profiling axis '{}'."_el}.build(selected, name));
                    }
                    auto copy = scenario;
                    copy.axes.append(AxisSelection{.axis = axis.id(), .value = selected});
                    next.append(std::move(copy));
                }
            }
            expanded = std::move(next);
        }
        for (const auto &expandedScenario : expanded) {
            auto scenario = expandedScenario;
            auto id = StringEditor{scenario.group};
            id.append(":"_el).append(scenario.functionality);
            for (const auto &axis : scenario.axes) {
                id.append(":"_el).append(axis.value);
            }
            for (const auto &parameter : definition.parameters()) {
                if (!entry->hasValue(parameter.configurationName)) {
                    if (parameter.defaultValue) {
                        scenario.parameters.append(
                            ParameterSelection{.parameter = parameter.id, .value = *parameter.defaultValue});
                    }
                    continue;
                }
                auto selection = ParameterSelection{.parameter = parameter.id};
                switch (parameter.type) {
                case ParameterType::Text:
                    selection.value = entry->getTextOrThrow(parameter.configurationName);
                    break;
                case ParameterType::Integer:
                    selection.value = entry->getIntegerOrThrow(parameter.configurationName);
                    break;
                case ParameterType::Boolean:
                    selection.value = entry->getBooleanOrThrow(parameter.configurationName);
                    break;
                case ParameterType::ByteLength: {
                    const auto value = entry->getIntegerOrThrow(parameter.configurationName);
                    if (value < 1) {
                        configurationError(StringFormat{"Parameter '{}' must be positive."_el}.build(parameter.id));
                    }
                    selection.value = ByteLength{static_cast<std::uint64_t>(value)};
                    break;
                }
                case ParameterType::TimeDelta:
                    selection.value = entry->getCalendarDeltaOrThrow(parameter.configurationName).toTimeDeltaOrThrow();
                    break;
                case ParameterType::Path:
                    selection.value = Path{entry->getTextOrThrow(parameter.configurationName)};
                    break;
                }
                scenario.parameters.append(std::move(selection));
            }
            scenario.id = String{id};
            if (!definition.isCompatible(scenario)) {
                configurationError(StringFormat{"Incompatible profiling scenario '{}'."_el}.build(scenario.id));
            }
            scenarios.append(std::move(scenario));
        }
    }
    return scenarios;
}

auto ConfigurationLoader::configurationDigest(const ProfilingConfiguration &configuration) -> ByteBlock {
    auto source = StringEditor{};
    source.append(configuration.run.suite).append(":"_el).append(StringEditor::fromInteger(configuration.run.seed));
    for (const auto &scenario : configuration.scenarios) {
        source.append("\n"_el)
            .append(scenario.id)
            .append(":"_el)
            .append(scenario.functionality)
            .append(":"_el)
            .append(StringEditor::fromInteger(scenario.weight));
        for (const auto &axis : scenario.axes) {
            source.append(":"_el).append(axis.axis).append("="_el).append(axis.value);
        }
        for (const auto &parameter : scenario.parameters) {
            source.append(":"_el).append(parameter.parameter).append("="_el);
            std::visit(
                [&](const auto &value) -> void {
                    using Value = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<Value, Path>) {
                        source.append(value.toString());
                    } else {
                        source.append(StringFormat{"{}"_el}.build(value));
                    }
                },
                parameter.value);
        }
    }
    auto hasher = cryptology::Hasher{cryptology::HashAlgorithm::Md5};
    hasher.update(String{source});
    return hasher.finalize();
}

auto ConfigurationLoader::load(const ProfilingDefinition &definition, const std::optional<Path> &path)
    -> ProfilingConfiguration {
    auto parser = conf::Parser{};
    auto result = ProfilingConfiguration{};
    const auto defaults = parser.parseTextOrThrow(definition.defaultConfiguration());
    auto custom = parseDocument(definition, defaults, result.run);
    if (path) {
        custom = parseDocument(definition, parser.parseFileOrThrow(*path), result.run);
    }
    if (custom) {
        result.scenarios = std::move(*custom);
    } else {
        const auto *suite = definition.findSuite(result.run.suite);
        if (suite == nullptr) {
            configurationError(StringFormat{"Unknown profiling suite '{}'."}.build(result.run.suite));
        }
        result.scenarios = suite->scenarios;
    }
    auto normalizedScenarios = List<Scenario>{};
    for (const auto &source : result.scenarios) {
        auto scenario = source;
        if (scenario.id.isEmpty()) {
            configurationError("A profiling scenario has an empty stable identifier."_el);
        }
        if (scenario.weight == 0U) {
            configurationError(StringFormat{"Scenario '{}' has a zero weight."_el}.build(scenario.id));
        }
        if (definition.findFunctionality(scenario.functionality) == nullptr) {
            configurationError(
                StringFormat{"Scenario '{}' uses unknown functionality '{}'."_el}.build(
                    scenario.id, scenario.functionality));
        }
        for (const auto &selection : scenario.axes) {
            if (definition.findAxis(selection.axis) == nullptr) {
                configurationError(
                    StringFormat{"Scenario '{}' selects unknown axis '{}'."_el}.build(scenario.id, selection.axis));
            }
        }
        for (const auto &axis : definition.axes()) {
            auto matches = std::uint32_t{};
            for (const auto &selection : scenario.axes) {
                if (selection.axis == axis.id()) {
                    ++matches;
                    if (!axis.hasValue(selection.value)) {
                        configurationError(
                            StringFormat{"Scenario '{}' has unknown value '{}' for axis '{}'."_el}.build(
                                scenario.id, selection.value, axis.id()));
                    }
                }
            }
            if (matches != 1U) {
                configurationError(
                    StringFormat{"Scenario '{}' must select axis '{}' exactly once."_el}.build(scenario.id, axis.id()));
            }
        }
        for (const auto &parameter : definition.parameters()) {
            auto found = false;
            for (const auto &selection : scenario.parameters) {
                if (selection.parameter == parameter.id) {
                    found = true;
                    break;
                }
            }
            if (!found && parameter.defaultValue) {
                scenario.parameters.append(
                    ParameterSelection{.parameter = parameter.id, .value = *parameter.defaultValue});
            }
        }
        for (const auto &selection : scenario.parameters) {
            if (definition.findParameter(selection.parameter) == nullptr) {
                configurationError(
                    StringFormat{"Scenario '{}' sets unknown parameter '{}'."_el}.build(
                        scenario.id, selection.parameter));
            }
        }
        if (!definition.isCompatible(scenario)) {
            configurationError(StringFormat{"Incompatible profiling scenario '{}'."_el}.build(scenario.id));
        }
        normalizedScenarios.append(std::move(scenario));
    }
    result.scenarios = std::move(normalizedScenarios);
    if (result.scenarios.isEmpty()) {
        configurationError("The profiling configuration expands to no scenarios."_el);
    }
    auto ids = std::set<String>{};
    for (const auto &scenario : result.scenarios) {
        if (!ids.emplace(scenario.id).second) {
            configurationError(StringFormat{"Duplicate expanded scenario ID '{}'."}.build(scenario.id));
        }
    }
    result.digest = configurationDigest(result);
    return result;
}

void ConfigurationLoader::writeTemplate(const ProfilingDefinition &definition, const Path &path) {
    auto options = PathWriteTextOptions{StringEncoding::Utf8};
    options.setCreateParents(true).setBomMode(StringBomMode::Reject);
    path.content().writeTextOrThrow(definition.defaultConfiguration(), options);
}

void ConfigurationLoader::updateDigest(ProfilingConfiguration &configuration) {
    configuration.digest = configurationDigest(configuration);
}

}
