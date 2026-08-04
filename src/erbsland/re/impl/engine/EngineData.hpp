// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupNames.hpp"
#include "CharClassData.hpp"
#include "EngineData_fwd.hpp"
#include "Program.hpp"
#include "SequenceData.hpp"

#include "../Limits.hpp"

#include <cstddef>

namespace erbsland::re::impl {

/// Compiled regular-expression data consumed by the matching engine.
class EngineData {
public:
    SequenceData sequenceData;
    CharClassData charClassData;
    Program program;
    CaptureGroupNames captureGroupNames;
    std::size_t counterCount{limits::maximumCounterCount};
    bool hasAtomicGroups{false};
};

}
