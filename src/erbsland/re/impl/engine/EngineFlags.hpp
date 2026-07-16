// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::re::impl {

enum class EngineFlag : uint8_t {
    /// `MATCH` and `SUCCESS` require to be at the end of the input.
    FullMatch = 1U << 0U,
    /// Implement the most efficient search for the first match.
    /// For each new character, start a new (low priority) thread with the current position as start.
    FindFirst = 1U << 1U,
    /// Read CR/LF as a single LF.
    FoldCRLF = 1U << 2U,
    /// Enable atomic groups in the engine.
    AtomicGroups = 1U << 3U,
};

/// Flags controlling an engine invocation.
/// @tested{EngineBasicTest}
class EngineFlags : public util::EnumFlags<EngineFlag, EngineFlags> {
    using Base = util::EnumFlags<EngineFlag, EngineFlags>;

public:
    using Base::Base;
};

}
