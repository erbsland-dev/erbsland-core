// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfigurationLoader_fwd.hpp"
#include "ScenarioTemplate.hpp"

#include "../ProfileTypes.hpp"

#include <erbsland/conf/Value.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/StringLiteral.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace app::regex::impl {

using namespace el::text::literals;

/// Loads, validates, and expands regex-profiler configuration.
class ConfigurationLoader final {
    using ValuePtr = el::conf::ValuePtr;
    static constexpr auto cSeedKey = "seed"_el; ///< Random seed configuration key.

public:
    /// Select the optional file that extends the embedded configuration.
    /// @param path The external configuration file, or no file for embedded defaults.
    explicit ConfigurationLoader(std::optional<el::Path> path) : _path{std::move(path)} {}

public: // loading
    /// Load the embedded configuration and optionally extend it from a file.
    [[nodiscard]] auto load() const -> Configuration;
    /// Write the embedded default configuration.
    /// @param path Destination path.
    static void writeTemplate(const el::Path &path);

private:
    /// Raise a configuration error with the given message.
    [[noreturn]] static void configError(const el::String &message);
    /// Raise a configuration error at the given configuration value.
    [[noreturn]] static void configError(const el::String &message, const ValuePtr &value);
    /// Verify that a configuration value has only recognized child keys.
    static void requireKnownKeys(const ValuePtr &value, const std::span<const el::StringLiteral> known);
    /// Get a positive integer configuration value or its default.
    [[nodiscard]] static auto positiveInteger(
        const ValuePtr &value, const el::String &name, const std::uint64_t defaultValue, const bool allowZero = false)
        -> std::uint64_t;
    /// Parse the configured run mode.
    [[nodiscard]] static auto parseMode(const el::String &value) -> RunMode;
    /// Get a duration configuration value or its default.
    [[nodiscard]] static auto duration(
        const ValuePtr &value, const el::String &name, const std::chrono::nanoseconds defaultValue)
        -> std::chrono::nanoseconds;
    /// Parse the use cases selected for a scenario.
    [[nodiscard]] static auto parseUseCases(const ValuePtr &value) -> std::vector<UseCase>;
    /// Map a configured file encoding name to its input kind.
    [[nodiscard]] static auto inputForEncoding(const el::String &name) -> std::optional<InputKind>;
    /// Parse scenario input kinds and their file encodings.
    [[nodiscard]] static auto parseInputs(const ValuePtr &value, el::StringList &fileEncodings)
        -> std::vector<InputKind>;
    /// Parse one unexpanded scenario definition.
    [[nodiscard]] static auto parseScenario(
        const ValuePtr &value, const RunSettings &run, const el::Path &configurationDirectory) -> ScenarioTemplate;
    /// Expand a scenario template into its concrete scenarios.
    static void expandTemplate(const ScenarioTemplate &source, std::vector<Scenario> &target);
    /// Parse the run settings from a configuration value.
    static void parseRun(const ValuePtr &run, RunSettings &result);
    /// Parse a configuration document and append its scenario templates.
    static void parseDocument(
        const ValuePtr &document,
        RunSettings &run,
        std::vector<ScenarioTemplate> &templates,
        const bool replaceTemplates,
        const el::Path &configurationDirectory);

private:
    std::optional<el::Path> _path; ///< External configuration, when supplied.
};

}
