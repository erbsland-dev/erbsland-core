// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/CpLength.hpp"

#include <cstdint>
#include <limits>

namespace erbsland::re::impl::limits {

/// The maximum length of a group name.
static constexpr std::size_t maximumGroupNameLength = 100U;

/// The maximum number of capture groups
static constexpr std::size_t maximumCaptureGroupCount = 100U;

/// The maximum number of flag characters.
static constexpr std::size_t maximumFlagCount = 10U;

/// The maximum length (code-points) of a pattern.
static constexpr unit::CpLength maximumPatternLength{10'000'000U};

/// The maximum length (code-points) of a replacement-text.
static constexpr unit::CpLength maximumReplacementTextLength = maximumPatternLength;

/// The maximum numeric quantifier count
static constexpr std::size_t maximumQuantifierCount = 10'000U;

/// The maximum nesting depth of groups.
static constexpr std::size_t maximumGroupNestingDepth = 100U;

/// The maximum number of counters.
static constexpr std::size_t maximumCounterCount = 15U;

/// The maximum length of a sequence.
static constexpr std::size_t maximumSequenceLength = 1000U;

/// The maximum length of a character sequence
static constexpr std::size_t maximumCharacterSequenceLength = std::numeric_limits<uint16_t>::max() - 1;

/// The maximum number of alternatives in a group.
static constexpr std::size_t maximumAlternativeCount = 1000U;

/// The minimum number of characters to use as a dedicated character sequence.
static constexpr std::size_t minimumCharacterSequenceLength = 3U;

/// The maximum length of a program.
static constexpr std::size_t maximumProgramLength = std::numeric_limits<uint16_t>::max() - 1;

/// The maximum number of character classes.
static constexpr std::size_t maximumCharacterClassCount = std::numeric_limits<uint16_t>::max() - 1;

/// The maximum length of a line in assembler code.
static constexpr unit::CpLength maximumAssemblerLineLength{1000U};

/// The maximum number of active threads in the engine
static constexpr std::size_t maximumActiveThreads = 10'000U;

/// The largest number of atomic groups permitted within a pattern.
/// This quantity is constrained by the size of the atomic group mask.
static constexpr std::size_t maximumAtomicGroupCount = 64U;

}
