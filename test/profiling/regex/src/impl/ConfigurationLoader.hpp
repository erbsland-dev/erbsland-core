// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BuiltInCatalog.hpp"
#include "BuiltInConfiguration.hpp"
#include "ConfigurationLoader_fwd.hpp"
#include "DefaultConfiguration.hpp"
#include "ScenarioTemplate.hpp"

#include "../ProfileTypes.hpp"

#include <erbsland/conf/Parser.hpp>

#include <array>
#include <set>

namespace app::regex::impl {

using namespace el::text::literals;

/// Loads, validates, and expands regex-profiler configuration.
/// @notest{Covered by regex profiler CTest entries.}
class ConfigurationLoader final {
    using ValuePtr = el::conf::ValuePtr;

public:
    /// Load the embedded configuration and optionally extend it from a file.
    [[nodiscard]] static auto load(const std::optional<el::Path> &path) -> Configuration {
        auto parser = el::conf::Parser{};
        auto run = RunSettings{};
        auto templates = std::vector<ScenarioTemplate>{};
        parseDocument(parser.parseTextOrThrow(el::String{cDefaultConfigurationText}), run, templates, false, {});
        if (path) {
            parseDocument(parser.parseFileOrThrow(*path), run, templates, true, path->parent());
        }
        auto result = Configuration{.run = run, .scenarios = {}};
        if (templates.empty()) {
            result.scenarios = BuiltInConfiguration::suite(result.run);
        } else {
            auto ids = std::set<el::String>{};
            for (const auto &source : templates) {
                const auto firstNewScenario = result.scenarios.size();
                expandTemplate(source, result.scenarios);
                for (auto index = firstNewScenario; index < result.scenarios.size(); ++index) {
                    const auto &scenario = result.scenarios[index];
                    if (!ids.emplace(scenario.id).second) {
                        configError(
                            el::StringFormat{"Duplicate expanded scenario ID '{}'."_el}.build(scenario.id),
                            source.sourceValue);
                    }
                }
            }
        }
        if (result.scenarios.empty()) {
            configError("The configuration expands to no scenarios."_el);
        }
        auto ids = std::set<el::String>{};
        for (const auto &scenario : result.scenarios) {
            if (!ids.emplace(scenario.id).second) {
                configError(el::StringFormat{"Duplicate expanded scenario ID '{}'."_el}.build(scenario.id));
            }
        }
        return result;
    }

    /// Write the embedded default configuration.
    /// @param path Destination path.
    static void writeTemplate(const el::Path &path) {
        auto options = el::PathWriteTextOptions{el::StringEncoding::Utf8};
        options.setCreateParents(true).setBomMode(el::StringBomMode::Reject);
        path.content().writeTextOrThrow(el::String{cDefaultConfigurationText}, options);
    }

private:
    /// Raise a configuration error with the given message.
    [[noreturn]] static void configError(const el::String &message) { throw el::ApplicationError{message}; }

    /// Raise a configuration error at the given configuration value.
    [[noreturn]] static void configError(const el::String &message, const ValuePtr &value) {
        throw el::conf::ConfError{
            el::conf::ConfErrorCategory::Validation, message, value->namePath(), value->location()};
    }

    /// Verify that a configuration value has only recognized child keys.
    static void requireKnownKeys(const ValuePtr &value, const std::span<const el::StringLiteral> known) {
        for (const auto &child : *value) {
            const auto childName = child->name().asText();
            if (std::ranges::none_of(known, [&](const auto name) { return childName == el::String{name}; })) {
                configError(el::StringFormat{"Unknown configuration key '{}'."_el}.build(childName), child);
            }
        }
    }

    /// Get a list value or return the supplied defaults.
    [[nodiscard]] static auto texts(const ValuePtr &value, const el::String &name, std::vector<el::String> defaults)
        -> std::vector<el::String> {
        return value->hasValue(name) ? value->valueOrThrow(name)->asListOrThrow<el::String>() : std::move(defaults);
    }

    /// Get a positive integer configuration value or its default.
    [[nodiscard]] static auto positiveInteger(
        const ValuePtr &value, const el::String &name, const std::uint64_t defaultValue, const bool allowZero = false)
        -> std::uint64_t {
        if (!value->hasValue(name)) {
            return defaultValue;
        }
        const auto result = value->getIntegerOrThrow(name);
        if (result < 0 || (!allowZero && result == 0)) {
            configError(
                el::StringFormat{"Configuration value '{}' must be {}."_el}.build(
                    name, allowZero ? "non-negative"_el : "positive"_el),
                value->valueOrThrow(name));
        }
        return static_cast<std::uint64_t>(result);
    }

    /// Parse the configured run mode.
    [[nodiscard]] static auto parseMode(const el::String &value) -> RunMode {
        if (value == "profile"_el) {
            return RunMode::Profile;
        }
        if (value == "benchmark"_el) {
            return RunMode::Benchmark;
        }
        configError(el::StringFormat{"Unsupported run mode '{}'."_el}.build(value));
    }

    /// Get a duration configuration value or its default.
    [[nodiscard]] static auto duration(
        const ValuePtr &value, const el::String &name, const std::chrono::nanoseconds defaultValue)
        -> std::chrono::nanoseconds {
        if (!value->hasValue(name)) {
            return defaultValue;
        }
        return value->getCalendarDeltaOrThrow(name).toTimeDeltaOrThrow().toStdNanoseconds();
    }

    /// Parse the use cases selected for a scenario.
    [[nodiscard]] static auto parseUseCases(const ValuePtr &value) -> std::vector<UseCase> {
        auto result = std::vector<UseCase>{};
        for (const auto &name : texts(value, "use_cases"_el, {"match"_el})) {
            const auto parsed = parseUseCase(name);
            if (!parsed) {
                configError(el::StringFormat{"Unsupported use case '{}'."_el}.build(name), value);
            }
            result.emplace_back(*parsed);
        }
        return result;
    }

    /// Map a configured file encoding name to its input kind.
    [[nodiscard]] static auto inputForEncoding(const el::String &name) -> std::optional<InputKind> {
        if (name == "utf8"_el) {
            return InputKind::FileUtf8;
        }
        if (name == "utf16"_el) {
            return InputKind::FileUtf16;
        }
        if (name == "utf32"_el) {
            return InputKind::FileUtf32;
        }
        return {};
    }

    /// Parse scenario input kinds and their file encodings.
    [[nodiscard]] static auto parseInputs(const ValuePtr &value, std::vector<el::String> &fileEncodings)
        -> std::vector<InputKind> {
        auto result = std::vector<InputKind>{};
        fileEncodings = texts(value, "file_encodings"_el, {"utf8"_el, "utf16"_el, "utf32"_el});
        for (const auto &name : texts(value, "inputs"_el, {"string-utf8"_el})) {
            if (name == "file"_el) {
                for (const auto &encoding : fileEncodings) {
                    const auto input = inputForEncoding(encoding);
                    if (!input) {
                        configError(el::StringFormat{"Unsupported file encoding '{}'."_el}.build(encoding), value);
                    }
                    result.emplace_back(*input);
                }
                continue;
            }
            const auto parsed = parseInputKind(name);
            if (!parsed) {
                configError(el::StringFormat{"Unsupported input kind '{}'."_el}.build(name), value);
            }
            result.emplace_back(*parsed);
        }
        std::ranges::sort(result, {}, [](const auto input) { return static_cast<std::uint8_t>(input); });
        result.erase(std::ranges::unique(result).begin(), result.end());
        return result;
    }

    /// Parse one unexpanded scenario definition.
    [[nodiscard]] static auto parseScenario(
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
        for (const auto &flagName : texts(value, "flags"_el, {})) {
            const auto flag = parseFlag(flagName);
            if (!flag) {
                configError(el::StringFormat{"Unsupported regular-expression flag '{}'."_el}.build(flagName), value);
            }
            result.flags.set(*flag);
        }
        const auto sourceCount = static_cast<unsigned>(value->hasValue("corpus"_el)) +
            static_cast<unsigned>(value->hasValue("subject"_el)) +
            static_cast<unsigned>(value->hasValue("source_file"_el));
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
        for (const auto &modeName : texts(value, "replacement_modes"_el, {"expression"_el})) {
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
                                el::StringFormat{
                                    "Incompatible use-case '{}', input '{}', and replacement mode '{}'."_el}
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

    /// Expand a scenario template into its concrete scenarios.
    static void expandTemplate(const ScenarioTemplate &source, std::vector<Scenario> &target) {
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

    /// Parse the run settings from a configuration value.
    static void parseRun(const ValuePtr &run, RunSettings &result) {
        constexpr auto keys = std::array{
            "mode"_el,
            "suite"_el,
            "duration"_el,
            "threads"_el,
            "seed"_el,
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
        const auto seed = run->getInteger("seed"_el, static_cast<el::conf::Integer>(result.seed));
        if (seed < 0) {
            configError("The global seed must not be negative."_el, run->valueOrThrow("seed"_el));
        }
        result.seed = static_cast<std::uint64_t>(seed);
        result.warmupSamples =
            static_cast<std::uint32_t>(positiveInteger(run, "warmup_samples"_el, result.warmupSamples, true));
        result.samples = static_cast<std::uint32_t>(positiveInteger(run, "samples"_el, result.samples));
        result.minimumSampleTime = duration(run, "minimum_sample_time"_el, result.minimumSampleTime);
        result.memoryLimit = positiveInteger(run, "memory_limit"_el, result.memoryLimit);
        result.progressInterval = duration(run, "progress_interval"_el, result.progressInterval);
        result.regexTimeout = std::chrono::duration_cast<std::chrono::milliseconds>(
            duration(run, "regex_timeout"_el, result.regexTimeout));
        result.workspace = run->getText("workspace"_el, result.workspace);
        result.keepFiles = run->getBoolean("keep_files"_el, result.keepFiles);
        if (result.duration <= std::chrono::nanoseconds::zero() ||
            result.minimumSampleTime <= std::chrono::nanoseconds::zero() ||
            result.progressInterval <= std::chrono::nanoseconds::zero() ||
            result.regexTimeout <= std::chrono::milliseconds::zero()) {
            configError("Run durations and intervals must be positive."_el);
        }
    }

    /// Parse a configuration document and append its scenario templates.
    static void parseDocument(
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
};

}
