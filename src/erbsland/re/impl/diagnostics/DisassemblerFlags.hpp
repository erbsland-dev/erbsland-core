// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DisassemblerFlag.hpp"

#include "../../../util/EnumFlags.hpp"

#include <cstdint>

namespace erbsland::re::impl {

/// Flags controlling diagnostic disassembly output.
/// @tested{DisassemblerTest}
class DisassemblerFlags : public util::EnumFlags<DisassemblerFlag, DisassemblerFlags> {
    using Base = util::EnumFlags<DisassemblerFlag, DisassemblerFlags>;

public:
    using Base::Base;
};

}
