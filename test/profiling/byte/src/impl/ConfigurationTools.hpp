// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ScenarioTemplate.hpp"

#include <erbsland/conf/Value.hpp>

namespace app::byte::impl {

/// Throw a profiling-configuration error.
[[noreturn]] void configError(const el::String &message);
/// Parse a profiling document into settings and scenario templates.
void parseDocument(
    const el::conf::ValuePtr &document,
    RunSettings &run,
    std::vector<ScenarioTemplate> &templates,
    bool replaceTemplates);
/// Create the built-in benchmark scenario suite.
[[nodiscard]] auto builtInSuite(const RunSettings &run) -> std::vector<Scenario>;
/// Expand a reusable template into concrete scenarios.
void expandTemplate(
    const ScenarioTemplate &source,
    const RunSettings &run,
    std::vector<Scenario> &target,
    const el::conf::ValuePtr &value);

}
