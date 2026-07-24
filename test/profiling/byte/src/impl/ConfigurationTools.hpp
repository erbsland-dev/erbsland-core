// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ScenarioTemplate.hpp"

#include <erbsland/conf/Value.hpp>

namespace app::byte::impl {

[[noreturn]] void configError(const el::String &message);
void parseDocument(
    const el::conf::ValuePtr &document,
    RunSettings &run,
    std::vector<ScenarioTemplate> &templates,
    bool replaceTemplates);
[[nodiscard]] auto builtInSuite(const RunSettings &run) -> std::vector<Scenario>;
void expandTemplate(
    const ScenarioTemplate &source,
    const RunSettings &run,
    std::vector<Scenario> &target,
    const el::conf::ValuePtr &value);

}
