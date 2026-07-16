// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupNames.hpp"
#include "CharClassData.hpp"
#include "Program.hpp"
#include "SequenceData.hpp"

#include <memory>

namespace erbsland::re::impl {

class EngineData;
using EngineDataPtr = std::shared_ptr<EngineData>;
using ConstEngineDataPtr = std::shared_ptr<const EngineData>;

class EngineData {
public:
    SequenceData sequenceData;
    CharClassData charClassData;
    Program program;
    CaptureGroupNames captureGroupNames;
    bool hasAtomicGroups{false};
};

}
