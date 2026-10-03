// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfigurationLoader.hpp"

#include "BuiltInCatalog.hpp"
#include "BuiltInConfiguration.hpp"

#include <erbsland/conf/Integer.hpp>
#include <erbsland/StringFormat.hpp>

#include <array>
#include <set>

namespace app::regex::impl {

auto ConfigurationLoader::parseScenario(
    const ValuePtr &value, const RunSettings &run, const el::Path &configurationDirectory) -> ScenarioTemplate {
    constexpr auto keys = std::array{
        "name"_el,
        "use_cases"_el,
        "inputs"_el,
        "file_encodings"_el,
        "pattern"_el,
        "pattern_name"_el,
        "flags"_el,
        "corpus"_el,
        "subject"_el,
        "source_file"_el,
        "repeat"_el,
        "replacement_modes"_el,
        "replacement"_el,
        "timeout"_el,
        "weight"_el};
    requireKnownKeys(value, keys);
    auto result = ScenarioTemplate{};
    result.sourceValue = value;
    result.name = value->getTextOrThrow("name"_el);
    result.useCases = parseUseCases(value);
    result.inputs = parseInputs(value, result.fileEncodings);
    const auto hasPattern = value->hasValue("pattern"_el);
    const auto hasPatternName = value->hasValue("pattern_name"_el);
    if (hasPattern == hasPatternName) {
        configError("A scenario must define exactly one of 'pattern' or 'pattern_name'."_el, value);
    }
    if (hasPattern) {
        const auto expression = value->getRegExOrThrow("pattern"_el);
        result.pattern = expression->pattern().toString();
        result.patternName = result.name;
    } else {
        result.patternName = value->getTextOrThrow("pattern_name"_el);
        const auto pattern = built_in_catalog::pattern(result.patternName);
        if (!pattern) {
            configError(el::StringFormat{"Unsupported built-in pattern '{}'."_el}.build(result.patternName), value);
        }
        result.pattern = *pattern;
    }
    for (const auto &flagName : value->getStringListOrThrow("flags"_el, el::StringList{})) {
        const auto flag = parseFlag(flagName);
        if (!flag) {
            configError(el::StringFormat{"Unsupported regular-expression flag '{}'."_el}.build(flagName), value);
        }
        result.flags.set(*flag);
    }
    const auto sourceCount = static_cast<unsigned>(value->hasValue("corpus"_el)) +
        static_cast<unsigned>(value->hasValue("subject"_el)) + static_cast<unsigned>(value->hasValue("source_file"_el));
    if (sourceCount > 1U) {
        configError("A scenario may define only one of 'corpus', 'subject', or 'source_file'."_el, value);
    }
    if (value->hasValue("corpus"_el)) {
        result.corpusName = value->getTextOrThrow("corpus"_el);
        if (const auto generated = built_in_catalog::generatedCorpus(result.corpusName)) {
            result.subject = *generated;
        } else if (const auto file = built_in_catalog::file(result.corpusName)) {
            result.corpusSource = CorpusSource::File;
            result.sourceFile = *file;
        } else {
            configError(el::StringFormat{"Unsupported built-in corpus '{}'."_el}.build(result.corpusName), value);
        }
    } else if (value->hasValue("source_file"_el)) {
        auto sourcePath = el::Path{value->getTextOrThrow("source_file"_el)};
        if (sourcePath.isRelative()) {
            sourcePath = configurationDirectory / sourcePath;
        }
        result.corpusName = sourcePath.name();
        result.corpusSource = CorpusSource::File;
        result.sourceFile = sourcePath.toString();
    } else {
        result.corpusName = value->hasValue("subject"_el) ? result.name : "empty"_el;
        result.subject = value->getText("subject"_el, {});
    }
    result.repetitionCount = static_cast<std::uint32_t>(positiveInteger(value, "repeat"_el, 1U));
    result.replacement = value->getText("replacement"_el, result.replacement);
    for (const auto &modeName : value->getStringListOrThrow("replacement_modes"_el, el::StringList{"expression"_el})) {
        const auto mode = parseReplacementMode(modeName);
        if (!mode) {
            configError(el::StringFormat{"Unsupported replacement mode '{}'."_el}.build(modeName), value);
        }
        result.replacementModes.emplace_back(*mode);
    }
    for (const auto useCase : result.useCases) {
        for (const auto input : result.inputs) {
            if (useCase == UseCase::ReplaceAll) {
                for (const auto replacementMode : result.replacementModes) {
                    if (!isCompatible(useCase, input, replacementMode)) {
                        configError(
                            el::StringFormat{"Incompatible use-case '{}', input '{}', and replacement mode '{}'."_el}
                                .build(toString(useCase), toString(input), toString(replacementMode)),
                            value);
                    }
                }
            } else if (!isCompatible(useCase, input, ReplacementMode::NotApplicable)) {
                configError(
                    el::StringFormat{"Incompatible use-case '{}' and input '{}'."_el}.build(
                        toString(useCase), toString(input)),
                    value);
            }
        }
    }
    result.timeout =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration(value, "timeout"_el, run.regexTimeout));
    result.weight = static_cast<std::uint32_t>(positiveInteger(value, "weight"_el, 1U));
    return result;
}

void ConfigurationLoader::expandTemplate(const ScenarioTemplate &source, std::vector<Scenario> &target) {
    for (const auto useCase : source.useCases) {
        for (const auto input : source.inputs) {
            if (useCase == UseCase::ReplaceAll) {
                for (const auto replacementMode : source.replacementModes) {
                    BuiltInConfiguration::addScenario(
                        target,
                        source.name,
                        useCase,
                        input,
                        source.patternName,
                        source.pattern,
                        source.corpusName,
                        source.corpusSource,
                        source.subject,
                        source.sourceFile,
                        replacementMode,
                        source.replacement,
                        source.flags,
                        source.timeout,
                        source.weight,
                        source.repetitionCount);
                }
            } else {
                BuiltInConfiguration::addScenario(
                    target,
                    source.name,
                    useCase,
                    input,
                    source.patternName,
                    source.pattern,
                    source.corpusName,
                    source.corpusSource,
                    source.subject,
                    source.sourceFile,
                    ReplacementMode::NotApplicable,
                    source.replacement,
                    source.flags,
                    source.timeout,
                    source.weight,
                    source.repetitionCount);
            }
        }
    }
}

void ConfigurationLoader::parseRun(const ValuePtr &run, RunSettings &result) {
    constexpr auto keys = std::array{
        "mode"_el,
        "suite"_el,
        "duration"_el,
        "threads"_el,
        cSeedKey,
        "warmup_samples"_el,
        "samples"_el,
        "minimum_sample_time"_el,
        "memory_limit"_el,
        "progress_interval"_el,
        "regex_timeout"_el,
        "workspace"_el,
        "keep_files"_el};
    requireKnownKeys(run, keys);
    result.mode = parseMode(run->getText("mode"_el, toString(result.mode)));
    result.suite = run->getText("suite"_el, result.suite);
    result.duration = duration(run, "duration"_el, result.duration);
    const auto threads = positiveInteger(run, "threads"_el, result.threadCount);
    if (threads > 256U) {
        configError("The workload thread count must be in the range 1-256."_el, run->valueOrThrow("threads"_el));
    }
    result.threadCount = static_cast<std::uint32_t>(threads);
    const auto seed = run->getInteger(cSeedKey, static_cast<el::conf::Integer>(result.seed));
    if (seed < 0) {
        configError("The global seed must not be negative."_el, run->valueOrThrow(cSeedKey));
    }
    result.seed = static_cast<std::uint64_t>(seed);
    result.warmupSamples =
        static_cast<std::uint32_t>(positiveInteger(run, "warmup_samples"_el, result.warmupSamples, true));
    result.samples = static_cast<std::uint32_t>(positiveInteger(run, "samples"_el, result.samples));
    result.minimumSampleTime = duration(run, "minimum_sample_time"_el, result.minimumSampleTime);
    result.memoryLimit = positiveInteger(run, "memory_limit"_el, result.memoryLimit);
    result.progressInterval = duration(run, "progress_interval"_el, result.progressInterval);
    result.regexTimeout =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration(run, "regex_timeout"_el, result.regexTimeout));
    result.workspace = run->getText("workspace"_el, result.workspace);
    result.keepFiles = run->getBoolean("keep_files"_el, result.keepFiles);
    if (result.duration <= std::chrono::nanoseconds::zero() ||
        result.minimumSampleTime <= std::chrono::nanoseconds::zero() ||
        result.progressInterval <= std::chrono::nanoseconds::zero() ||
        result.regexTimeout <= std::chrono::milliseconds::zero()) {
        configError("Run durations and intervals must be positive."_el);
    }
}

void ConfigurationLoader::parseDocument(
    const ValuePtr &document,
    RunSettings &run,
    std::vector<ScenarioTemplate> &templates,
    const bool replaceTemplates,
    const el::Path &configurationDirectory) {
    constexpr auto rootKeys = std::array{"run"_el, "scenario"_el};
    requireKnownKeys(document, rootKeys);
    if (document->hasValue("run"_el)) {
        parseRun(document->valueOrThrow("run"_el), run);
    }
    if (document->hasValue("scenario"_el)) {
        if (replaceTemplates) {
            templates.clear();
        }
        for (const auto &entry : *document->valueOrThrow("scenario"_el)) {
            templates.emplace_back(parseScenario(entry, run, configurationDirectory));
        }
    }
}

}
