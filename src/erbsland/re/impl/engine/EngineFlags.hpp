// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EngineFlag.hpp"

#include "../../../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::re::impl {

/// Flags controlling an engine invocation.
/// @tested{EngineBasicTest}
class EngineFlags : public util::EnumFlags<EngineFlag, EngineFlags> {
    using Base = util::EnumFlags<EngineFlag, EngineFlags>;

public:
    using Base::Base;
};

}
