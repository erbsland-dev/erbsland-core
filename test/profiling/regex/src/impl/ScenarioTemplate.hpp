// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ProfileTypes.hpp"

#include <erbsland/conf/Parser.hpp>

#include <chrono>
#include <cstdint>
#include <vector>

namespace app::regex::impl {

using namespace el::text::literals;

/// Stores one unexpanded profiling scenario definition.
/// @notest{Covered by ConfigurationLoader profiling tests.}
struct ScenarioTemplate {
    el::conf::ValuePtr sourceValue;
    el::String name;
    std::vector<UseCase> useCases;
    std::vector<InputKind> inputs;
    std::vector<el::String> fileEncodings;
    el::String patternName;
    el::String pattern;
    el::re::Flags flags;
    el::String corpusName;
    CorpusSource corpusSource{CorpusSource::Inline};
    el::String subject;
    el::String sourceFile;
    std::uint32_t repetitionCount{1U};
    std::vector<ReplacementMode> replacementModes;
    el::String replacement{"{0}"_el};
    std::chrono::milliseconds timeout{std::chrono::seconds{30}};
    std::uint32_t weight{1U};
};

}
