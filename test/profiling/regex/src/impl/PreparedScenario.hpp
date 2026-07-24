// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PreparedScenario_fwd.hpp"

#include "../ProfileTypes.hpp"

namespace app::regex::impl {

/// A scenario with all immutable input fixtures prepared.
/// @notest{Verified by regex profiler smoke CTest entries.}
struct PreparedScenario {
    Scenario scenario;                 ///< Expanded source scenario.
    el::text::AnyString pattern;       ///< Pattern in the measured string representation.
    el::String subject8;               ///< UTF-8 subject.
    el::text::U16String subject16;     ///< UTF-16 subject.
    el::text::U32String subject32;     ///< UTF-32 subject.
    el::Path filePath;                 ///< Prepared file for stream inputs.
    el::re::RegExPtr expression;       ///< Shared eager expression for execution cases.
    std::uint64_t validationSink{};    ///< Width-independent canonical result signature.
    std::uint64_t validationMatches{}; ///< Matches in one validation operation.
};

}
