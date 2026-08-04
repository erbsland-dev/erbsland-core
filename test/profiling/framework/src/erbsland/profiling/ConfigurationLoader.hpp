// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfigurationLoader_fwd.hpp"
#include "ProfilingConfiguration.hpp"
#include "ProfilingDefinition.hpp"

#include <erbsland/all.hpp>
#include <erbsland/conf/Value.hpp>

#include <optional>

namespace erbsland::profiling {

/// Load, validate, and expand declarative profiler configuration.
/// @tested{ConfigurationLoaderTest}
class ConfigurationLoader final {
public:
    /// Load embedded defaults and an optional overriding file.
    /// @param definition Profiling application definition.
    /// @param path Optional external ELCL file.
    /// @return Expanded effective configuration.
    [[nodiscard]] static auto load(const ProfilingDefinition &definition, const std::optional<Path> &path = {})
        -> ProfilingConfiguration;
    /// Write the embedded configuration template.
    /// @param definition Profiling application definition.
    /// @param path Destination path.
    static void writeTemplate(const ProfilingDefinition &definition, const Path &path);
    /// Recalculate the stable digest after command-line overrides and filtering.
    /// @param configuration Effective configuration to update.
    static void updateDigest(ProfilingConfiguration &configuration);

private:
    /// Throw a configuration error with contextual text.
    [[noreturn]] static void configurationError(const String &message);
    /// Require that a value belongs to a known set of names.
    static void requireKnown(const conf::ValuePtr &value, const StringList &known);
    /// Parse one run configuration from a document value.
    static void parseRun(const conf::ValuePtr &value, RunConfiguration &run);
    /// Parse a profiling document into optional scenarios.
    [[nodiscard]] static auto parseDocument(
        const ProfilingDefinition &definition, const conf::ValuePtr &document, RunConfiguration &run)
        -> std::optional<List<Scenario>>;
    /// Produce the stable digest of an effective configuration.
    [[nodiscard]] static auto configurationDigest(const ProfilingConfiguration &configuration) -> ByteBlock;
};

}
